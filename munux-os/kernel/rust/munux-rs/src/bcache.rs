//! Buffer cache: cache de setores de disco (512 bytes) sobre o driver ATA em C.
//!
//! Leituras consultam o cache primeiro; em miss, buscam no disco e inserem.
//! Escritas são write-through (disco + cache). Mantém contadores de hit/miss
//! para o smoke test. Política de evicção simples (bounded) — o suficiente para
//! um primeiro cache; LRU fica para depois.

#![allow(clippy::cast_possible_truncation, clippy::must_use_candidate)]

use alloc::collections::BTreeMap;

use crate::sync::IrqMutex;

extern "C" {
    fn disk_read_sector(lba: u32, buf: *mut u8) -> i32;
    fn disk_write_sector(lba: u32, buf: *const u8) -> i32;
}

const SECTOR: usize = 512;
const MAX_ENTRIES: usize = 256; // teto de setores em cache (128 KiB)

struct Cache {
    map: BTreeMap<u32, [u8; SECTOR]>,
    hits: u32,
    misses: u32,
}

static CACHE: IrqMutex<Option<Cache>> = IrqMutex::new(None);

fn with_cache<R>(f: impl FnOnce(&mut Cache) -> R) -> R {
    let mut guard = CACHE.lock();
    if guard.is_none() {
        *guard = Some(Cache {
            map: BTreeMap::new(),
            hits: 0,
            misses: 0,
        });
    }
    // SAFETY-free: acabamos de garantir que é `Some`.
    f(guard.as_mut().unwrap())
}

/// Lê o setor `lba` (512 bytes) para `out`, servindo do cache quando possível.
pub fn read_sector(lba: u32, out: &mut [u8; SECTOR]) {
    with_cache(|c| {
        if let Some(block) = c.map.get(&lba).copied() {
            c.hits += 1;
            *out = block;
            return;
        }
        c.misses += 1;
        let mut block = [0u8; SECTOR];
        // SAFETY: `block` tem 512 bytes; `disk_read_sector` escreve 512.
        unsafe {
            disk_read_sector(lba, block.as_mut_ptr());
        }
        if c.map.len() >= MAX_ENTRIES {
            if let Some(&victim) = c.map.keys().next() {
                c.map.remove(&victim);
            }
        }
        c.map.insert(lba, block);
        *out = block;
    });
}

/// Escreve o setor `lba` (write-through: disco + cache).
pub fn write_sector(lba: u32, data: &[u8; SECTOR]) {
    // SAFETY: `data` tem 512 bytes; `disk_write_sector` lê 512.
    unsafe {
        disk_write_sector(lba, data.as_ptr());
    }
    with_cache(|c| {
        c.map.insert(lba, *data);
    });
}

/// Contadores acumulados (hits, misses).
pub fn stats() -> (u32, u32) {
    with_cache(|c| (c.hits, c.misses))
}

/// Smoke test: lê o mesmo setor duas vezes; a 2ª deve ser hit e igual à 1ª.
/// Requer um disco anexado (senão os dados são indefinidos, mas o teste de
/// hit/miss ainda vale). Chamado apenas quando há disco.
pub fn selftest() -> bool {
    let (h0, m0) = stats();
    let mut a = [0u8; SECTOR];
    read_sector(2, &mut a); // miss (setor do superbloco)
    let mut b = [0u8; SECTOR];
    read_sector(2, &mut b); // hit
    let (h1, m1) = stats();
    a == b && h1 > h0 && m1 > m0
}
