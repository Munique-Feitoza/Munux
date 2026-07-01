//! Leitor de ext2 (somente leitura).
//!
//! Lê o superbloco, os descritores de grupo, inodes e diretórios através do
//! buffer cache ([`crate::bcache`]), que por sua vez usa o driver ATA em C.
//! Suporta montar uma imagem ext2 real, listar diretórios e ler arquivos com
//! ponteiros diretos e **indiretos** (simples e duplo). Escrita, links e blocos
//! triplamente indiretos ficam para depois.

#![allow(
    clippy::cast_lossless,
    clippy::cast_possible_truncation,
    clippy::missing_panics_doc,
    clippy::must_use_candidate
)]

use alloc::string::String;
use alloc::vec::Vec;

const SECTOR: usize = 512;
const EXT2_MAGIC: u16 = 0xEF53;
const ROOT_INO: u32 = 2;

fn rd_u16(b: &[u8], off: usize) -> u16 {
    u16::from_le_bytes([b[off], b[off + 1]])
}

fn rd_u32(b: &[u8], off: usize) -> u32 {
    u32::from_le_bytes([b[off], b[off + 1], b[off + 2], b[off + 3]])
}

/// Lê `len` bytes a partir de `byte_off`, cobrindo os setores necessários.
fn read_range(byte_off: u32, len: usize) -> Vec<u8> {
    let start = byte_off as usize / SECTOR;
    let end = (byte_off as usize + len).div_ceil(SECTOR);
    let mut raw = Vec::with_capacity((end - start) * SECTOR);
    for sec in start..end {
        let mut sbuf = [0u8; SECTOR];
        crate::bcache::read_sector(sec as u32, &mut sbuf);
        raw.extend_from_slice(&sbuf);
    }
    let skip = byte_off as usize % SECTOR;
    raw[skip..skip + len].to_vec()
}

/// Inode simplificado (ponteiros diretos + indireto simples e duplo).
struct Inode {
    size: u32,
    blocks: [u32; 12],
    indirect: u32,
    double_indirect: u32,
}

/// Parâmetros do sistema de arquivos extraídos do superbloco.
struct Fs {
    block_size: u32,
    inodes_per_group: u32,
    inode_size: u32,
    bgdt_off: u32, // byte offset da tabela de descritores de grupo
}

fn read_super() -> Option<Fs> {
    let sb = read_range(1024, 1024);
    if rd_u16(&sb, 56) != EXT2_MAGIC {
        return None;
    }
    let block_size = 1024u32 << rd_u32(&sb, 24);
    let inodes_per_group = rd_u32(&sb, 40);
    let rev = rd_u32(&sb, 76);
    let inode_size = if rev >= 1 {
        rd_u16(&sb, 88) as u32
    } else {
        128
    };
    // A tabela de descritores começa no bloco 2 se block_size==1024, senão no 1.
    let bgdt_block = if block_size == 1024 { 2 } else { 1 };
    Some(Fs {
        block_size,
        inodes_per_group,
        inode_size,
        bgdt_off: bgdt_block * block_size,
    })
}

fn read_inode(fs: &Fs, ino: u32) -> Inode {
    let group = (ino - 1) / fs.inodes_per_group;
    let index = (ino - 1) % fs.inodes_per_group;
    let desc = read_range(fs.bgdt_off + group * 32, 32);
    let inode_table = rd_u32(&desc, 8);
    let off = inode_table * fs.block_size + index * fs.inode_size;
    let raw = read_range(off, fs.inode_size as usize);
    let mut blocks = [0u32; 12];
    for (k, b) in blocks.iter_mut().enumerate() {
        *b = rd_u32(&raw, 40 + 4 * k);
    }
    Inode {
        size: rd_u32(&raw, 4),
        blocks,
        indirect: rd_u32(&raw, 88),        // i_block[12]
        double_indirect: rd_u32(&raw, 92), // i_block[13]
    }
}

/// Número físico do bloco lógico `n` do arquivo (direto, indireto simples e
/// duplo). Retorna 0 se o bloco não existir.
fn block_at(fs: &Fs, inode: &Inode, n: usize) -> u32 {
    let ppb = fs.block_size as usize / 4; // ponteiros por bloco
    if n < 12 {
        return inode.blocks[n];
    }
    let n = n - 12;
    if n < ppb {
        if inode.indirect == 0 {
            return 0;
        }
        let tbl = read_range(inode.indirect * fs.block_size, fs.block_size as usize);
        return rd_u32(&tbl, n * 4);
    }
    let n = n - ppb;
    if inode.double_indirect == 0 {
        return 0;
    }
    let dbl = read_range(
        inode.double_indirect * fs.block_size,
        fs.block_size as usize,
    );
    let outer = rd_u32(&dbl, (n / ppb) * 4);
    if outer == 0 {
        return 0;
    }
    let inner = read_range(outer * fs.block_size, fs.block_size as usize);
    rd_u32(&inner, (n % ppb) * 4)
}

fn read_dir(fs: &Fs, inode: &Inode) -> Vec<(String, u32)> {
    let mut out = Vec::new();
    for &blk in &inode.blocks {
        if blk == 0 {
            continue;
        }
        let data = read_range(blk * fs.block_size, fs.block_size as usize);
        let mut pos = 0usize;
        while pos + 8 <= data.len() {
            let inode_num = rd_u32(&data, pos);
            let rec_len = rd_u16(&data, pos + 4) as usize;
            let name_len = data[pos + 6] as usize;
            if rec_len == 0 {
                break;
            }
            if inode_num != 0 && name_len > 0 && pos + 8 + name_len <= data.len() {
                if let Ok(name) = core::str::from_utf8(&data[pos + 8..pos + 8 + name_len]) {
                    out.push((String::from(name), inode_num));
                }
            }
            pos += rec_len;
        }
    }
    out
}

fn read_file(fs: &Fs, inode: &Inode) -> Vec<u8> {
    let mut out = Vec::new();
    let mut remaining = inode.size as usize;
    let nblocks = (inode.size as usize).div_ceil(fs.block_size as usize);
    for n in 0..nblocks {
        if remaining == 0 {
            break;
        }
        let blk = block_at(fs, inode, n);
        if blk == 0 {
            break;
        }
        let take = core::cmp::min(remaining, fs.block_size as usize);
        out.extend_from_slice(&read_range(blk * fs.block_size, take));
        remaining -= take;
    }
    out
}

/// Smoke test: monta a imagem ext2 do disco, lista `/`, acha `hello.txt` e
/// confere seu conteúdo. Retorna `true` se tudo bateu. Requer uma imagem ext2
/// válida no Primary Master.
pub fn selftest() -> bool {
    let Some(fs) = read_super() else {
        return false;
    };
    let root = read_inode(&fs, ROOT_INO);
    let entries = read_dir(&fs, &root);

    // Arquivo pequeno (blocos diretos).
    let Some(&(_, ino)) = entries.iter().find(|(name, _)| name == "hello.txt") else {
        return false;
    };
    if read_file(&fs, &read_inode(&fs, ino)) != b"Hello from Munux ext2!\n" {
        return false;
    }

    // Arquivo grande (blocos indiretos), se presente: byte[i] = i & 0xFF.
    if let Some(&(_, big)) = entries.iter().find(|(name, _)| name == "big.bin") {
        let data = read_file(&fs, &read_inode(&fs, big));
        if data.len() != 20480 {
            return false;
        }
        for (i, &b) in data.iter().enumerate() {
            if b != (i & 0xFF) as u8 {
                return false;
            }
        }
    }

    true
}
