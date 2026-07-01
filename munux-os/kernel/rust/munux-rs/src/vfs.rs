//! Virtual File System (VFS) e um `tmpfs` em memória.
//!
//! O [`Filesystem`] é o contrato mínimo que qualquer backend implementa
//! (criar, ler, escrever, listar). O [`TmpFs`] é a primeira implementação:
//! uma árvore em RAM, montada como raiz (`/`), usada nos testes de boot.
//!
//! Representação do `TmpFs`: um mapa plano de caminho absoluto → nó. Simples e
//! correto para um primeiro sistema de arquivos; sem resolução de `.`/`..` nem
//! links por enquanto (isso fica para o ext2).

#![allow(
    clippy::missing_errors_doc,
    clippy::must_use_candidate,
    clippy::cast_possible_truncation,
    clippy::cast_possible_wrap
)]

use alloc::collections::BTreeMap;
use alloc::string::{String, ToString};
use alloc::vec::Vec;

use crate::sync::IrqMutex;

/// Erros do VFS.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum VfsError {
    NotFound,
    AlreadyExists,
    NotADirectory,
    IsADirectory,
    InvalidPath,
}

/// Resultado de operações do VFS.
pub type Result<T> = core::result::Result<T, VfsError>;

/// Tipo de nó do sistema de arquivos.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum NodeKind {
    File,
    Dir,
}

/// Metadados de um nó — o "inode" do VFS (tipo e tamanho).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Metadata {
    pub kind: NodeKind,
    pub size: usize,
}

/// Contrato mínimo que qualquer sistema de arquivos do Munux implementa.
pub trait Filesystem {
    /// Cria um nó (arquivo ou diretório) em `path`. O pai deve existir e ser
    /// um diretório.
    fn create(&mut self, path: &str, kind: NodeKind) -> Result<()>;
    /// Substitui o conteúdo do arquivo em `path` por `data`, devolvendo o
    /// número de bytes escritos.
    fn write(&mut self, path: &str, data: &[u8]) -> Result<usize>;
    /// Copia o conteúdo do arquivo em `path` para `out`, devolvendo quantos
    /// bytes foram copiados (`min(tamanho, out.len())`).
    fn read(&self, path: &str, out: &mut [u8]) -> Result<usize>;
    /// Lista os nomes dos filhos diretos do diretório em `path`.
    fn list(&self, path: &str) -> Result<Vec<String>>;
    /// Retorna `true` se `path` existe.
    fn exists(&self, path: &str) -> bool;
    /// Metadados (tipo e tamanho) do nó em `path` — o "inode" do VFS.
    fn stat(&self, path: &str) -> Result<Metadata>;
}

/// Nó de um `tmpfs`: um arquivo (bytes) ou um diretório.
enum Node {
    File(Vec<u8>),
    Dir,
}

/// Sistema de arquivos em memória. A serialização é externa: o acesso passa
/// por [`ROOT`], então os métodos não travam nada internamente.
pub struct TmpFs {
    nodes: BTreeMap<String, Node>,
}

impl TmpFs {
    /// Cria um `tmpfs` vazio contendo apenas a raiz `/`.
    pub fn new() -> Self {
        let mut nodes = BTreeMap::new();
        nodes.insert(String::from("/"), Node::Dir);
        Self { nodes }
    }

    /// Cópia do conteúdo do arquivo em `path`, ou `None` se não for arquivo.
    fn contents(&self, path: &str) -> Option<Vec<u8>> {
        let path = clean(path).ok()?;
        match self.nodes.get(path) {
            Some(Node::File(buf)) => Some(buf.clone()),
            _ => None,
        }
    }
}

impl Default for TmpFs {
    fn default() -> Self {
        Self::new()
    }
}

/// Normaliza um caminho absoluto: exige começar com `/` e remove uma barra
/// final redundante (exceto na raiz).
fn clean(path: &str) -> Result<&str> {
    if !path.starts_with('/') {
        return Err(VfsError::InvalidPath);
    }
    if path.len() > 1 && path.ends_with('/') {
        Ok(&path[..path.len() - 1])
    } else {
        Ok(path)
    }
}

/// Caminho do diretório pai de um caminho já normalizado (não-raiz).
fn parent_of(path: &str) -> &str {
    match path.rfind('/') {
        Some(0) | None => "/",
        Some(i) => &path[..i],
    }
}

/// Último componente do caminho (nome do arquivo/diretório).
fn basename(path: &str) -> &str {
    match path.rfind('/') {
        Some(i) => &path[i + 1..],
        None => path,
    }
}

impl Filesystem for TmpFs {
    fn create(&mut self, path: &str, kind: NodeKind) -> Result<()> {
        let path = clean(path)?;
        if self.nodes.contains_key(path) {
            return Err(VfsError::AlreadyExists);
        }
        match self.nodes.get(parent_of(path)) {
            Some(Node::Dir) => {}
            Some(Node::File(_)) => return Err(VfsError::NotADirectory),
            None => return Err(VfsError::NotFound),
        }
        let node = match kind {
            NodeKind::File => Node::File(Vec::new()),
            NodeKind::Dir => Node::Dir,
        };
        self.nodes.insert(path.to_string(), node);
        Ok(())
    }

    fn write(&mut self, path: &str, data: &[u8]) -> Result<usize> {
        let path = clean(path)?;
        match self.nodes.get_mut(path) {
            Some(Node::File(buf)) => {
                buf.clear();
                buf.extend_from_slice(data);
                Ok(data.len())
            }
            Some(Node::Dir) => Err(VfsError::IsADirectory),
            None => Err(VfsError::NotFound),
        }
    }

    fn read(&self, path: &str, out: &mut [u8]) -> Result<usize> {
        let path = clean(path)?;
        match self.nodes.get(path) {
            Some(Node::File(buf)) => {
                let n = core::cmp::min(buf.len(), out.len());
                out[..n].copy_from_slice(&buf[..n]);
                Ok(n)
            }
            Some(Node::Dir) => Err(VfsError::IsADirectory),
            None => Err(VfsError::NotFound),
        }
    }

    fn list(&self, path: &str) -> Result<Vec<String>> {
        let path = clean(path)?;
        match self.nodes.get(path) {
            Some(Node::Dir) => {}
            Some(Node::File(_)) => return Err(VfsError::NotADirectory),
            None => return Err(VfsError::NotFound),
        }
        let mut out = Vec::new();
        for key in self.nodes.keys() {
            if key.as_str() != path && parent_of(key) == path {
                out.push(basename(key).to_string());
            }
        }
        Ok(out)
    }

    fn exists(&self, path: &str) -> bool {
        match clean(path) {
            Ok(p) => self.nodes.contains_key(p),
            Err(_) => false,
        }
    }

    fn stat(&self, path: &str) -> Result<Metadata> {
        let path = clean(path)?;
        match self.nodes.get(path) {
            Some(Node::File(buf)) => Ok(Metadata {
                kind: NodeKind::File,
                size: buf.len(),
            }),
            Some(Node::Dir) => Ok(Metadata {
                kind: NodeKind::Dir,
                size: 0,
            }),
            None => Err(VfsError::NotFound),
        }
    }
}

/// A raiz montada do VFS. `None` até [`mount_tmpfs_root`] rodar.
static ROOT: IrqMutex<Option<TmpFs>> = IrqMutex::new(None);

/// Monta um `tmpfs` vazio como raiz (`/`), substituindo o que houver.
pub fn mount_tmpfs_root() {
    *ROOT.lock() = Some(TmpFs::new());
}

/// Smoke test do VFS: monta um `tmpfs`, cria `/etc/motd`, escreve, lê de volta,
/// confere as listagens de `/etc` e `/`, e valida os erros esperados. Retorna
/// `true` se tudo bateu. Precisa do heap já inicializado.
pub fn selftest() -> bool {
    mount_tmpfs_root();
    let mut guard = ROOT.lock();
    let Some(fs) = guard.as_mut() else {
        return false;
    };

    if fs.create("/etc", NodeKind::Dir).is_err() {
        return false;
    }
    if fs.create("/etc/motd", NodeKind::File).is_err() {
        return false;
    }

    let msg = b"Bem-vindo ao Munux";
    if fs.write("/etc/motd", msg) != Ok(msg.len()) {
        return false;
    }

    let mut buf = [0u8; 64];
    let Ok(n) = fs.read("/etc/motd", &mut buf) else {
        return false;
    };
    if &buf[..n] != msg {
        return false;
    }

    // stat (o "inode" do VFS): tipo e tamanho.
    match fs.stat("/etc/motd") {
        Ok(m) if m.kind == NodeKind::File && m.size == msg.len() => {}
        _ => return false,
    }
    if fs.stat("/etc").map(|m| m.kind) != Ok(NodeKind::Dir) {
        return false;
    }

    let Ok(etc) = fs.list("/etc") else {
        return false;
    };
    if !etc.iter().any(|e| e == "motd") {
        return false;
    }

    let Ok(root) = fs.list("/") else {
        return false;
    };
    if !root.iter().any(|e| e == "etc") {
        return false;
    }

    // Erros esperados.
    if fs.create("/etc", NodeKind::Dir) != Err(VfsError::AlreadyExists) {
        return false;
    }
    if fs.read("/nope", &mut buf) != Err(VfsError::NotFound) {
        return false;
    }

    true
}

/// Tabela de descritores abertos: fd → (caminho, offset de leitura).
static FDS: IrqMutex<Option<BTreeMap<i32, (String, usize)>>> = IrqMutex::new(None);

/// Monta o `tmpfs` raiz para uso, popula `/etc/motd` e zera a tabela de
/// descritores. Chamado uma vez no boot (depois do smoke test do VFS).
pub fn init_default() {
    mount_tmpfs_root();
    {
        let mut guard = ROOT.lock();
        if let Some(fs) = guard.as_mut() {
            let _ = fs.create("/etc", NodeKind::Dir);
            let _ = fs.create("/etc/motd", NodeKind::File);
            let _ = fs.write("/etc/motd", b"Munux v0.4\n");
        }
    }
    *FDS.lock() = Some(BTreeMap::new());
}

/// Abre o arquivo em `path`, devolvendo um descritor (>= 3) ou -1.
pub fn fd_open(path: &str) -> i32 {
    {
        let guard = ROOT.lock();
        if guard.as_ref().is_none_or(|fs| fs.contents(path).is_none()) {
            return -1;
        }
    }
    let mut guard = FDS.lock();
    let Some(table) = guard.as_mut() else {
        return -1;
    };
    let mut fd = 3;
    while table.contains_key(&fd) {
        fd += 1;
    }
    table.insert(fd, (String::from(path), 0));
    fd
}

/// Lê até `out.len()` bytes do descritor `fd`. Retorna bytes lidos, 0 no EOF,
/// ou -1 em erro (descritor inválido).
pub fn fd_read(fd: i32, out: &mut [u8]) -> i32 {
    let (path, offset) = {
        let guard = FDS.lock();
        match guard.as_ref().and_then(|t| t.get(&fd)) {
            Some((p, o)) => (p.clone(), *o),
            None => return -1,
        }
    };
    let content = {
        let guard = ROOT.lock();
        match guard.as_ref().and_then(|fs| fs.contents(&path)) {
            Some(c) => c,
            None => return -1,
        }
    };
    if offset >= content.len() {
        return 0;
    }
    let n = core::cmp::min(out.len(), content.len() - offset);
    out[..n].copy_from_slice(&content[offset..offset + n]);
    if let Some(t) = FDS.lock().as_mut() {
        if let Some((_, o)) = t.get_mut(&fd) {
            *o += n;
        }
    }
    n as i32
}

/// Fecha o descritor `fd`. Retorna 0 em sucesso, -1 se não existir.
pub fn fd_close(fd: i32) -> i32 {
    match FDS.lock().as_mut().and_then(|t| t.remove(&fd)) {
        Some(_) => 0,
        None => -1,
    }
}
