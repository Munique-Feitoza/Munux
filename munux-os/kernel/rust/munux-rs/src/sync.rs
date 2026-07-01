//! Synchronization primitives that respect the kernel's single-CPU,
//! interrupt-preemptible execution model.
//!
//! The kernel is currently uniprocessor, but interrupt handlers can
//! preempt thread context at any time. A naive spinlock is therefore
//! insufficient: a handler that tries to acquire a lock already held
//! by the preempted thread would deadlock.
//!
//! [`IrqMutex`] solves this by disabling local interrupts for the
//! duration of the critical section. When SMP arrives, the contained
//! [`SpinLock`] will already provide the cross-CPU guarantee.

use core::cell::UnsafeCell;
use core::ops::{Deref, DerefMut};
use core::sync::atomic::{AtomicBool, Ordering};

extern "C" {
    fn munux_c_irq_save() -> u32;
    fn munux_c_irq_restore(flags: u32);
}

/// A test-and-set spinlock. Used as the inner mechanism of [`IrqMutex`]
/// and ready to become the primary primitive once SMP lands.
struct SpinLock {
    locked: AtomicBool,
}

impl SpinLock {
    const fn new() -> Self {
        Self {
            locked: AtomicBool::new(false),
        }
    }

    fn acquire(&self) {
        while self
            .locked
            .compare_exchange_weak(false, true, Ordering::Acquire, Ordering::Relaxed)
            .is_err()
        {
            core::hint::spin_loop();
        }
    }

    fn release(&self) {
        self.locked.store(false, Ordering::Release);
    }
}

/// A mutex that disables local IRQs while held.
///
/// Safe to acquire from both thread and interrupt context.
pub struct IrqMutex<T> {
    inner: SpinLock,
    data: UnsafeCell<T>,
}

// SAFETY: access to the inner value is serialized through the spinlock
// and IRQs are disabled for the duration of the critical section. The
// only mutation paths go through `lock`, which returns an `IrqGuard`
// that holds the lock across its entire lifetime.
unsafe impl<T: Send> Sync for IrqMutex<T> {}

impl<T> IrqMutex<T> {
    pub const fn new(value: T) -> Self {
        Self {
            inner: SpinLock::new(),
            data: UnsafeCell::new(value),
        }
    }

    pub fn lock(&self) -> IrqGuard<'_, T> {
        // SAFETY: this extern call is a kernel entry point that pushes
        // EFLAGS and clears IF. It has no Rust-side preconditions.
        let flags = unsafe { munux_c_irq_save() };
        self.inner.acquire();
        IrqGuard { mutex: self, flags }
    }
}

pub struct IrqGuard<'a, T> {
    mutex: &'a IrqMutex<T>,
    flags: u32,
}

impl<T> Deref for IrqGuard<'_, T> {
    type Target = T;
    fn deref(&self) -> &T {
        // SAFETY: the lock is held for the lifetime of this guard.
        unsafe { &*self.mutex.data.get() }
    }
}

impl<T> DerefMut for IrqGuard<'_, T> {
    fn deref_mut(&mut self) -> &mut T {
        // SAFETY: the lock is held for the lifetime of this guard, and
        // a guard exposes mutable access exclusively to its holder.
        unsafe { &mut *self.mutex.data.get() }
    }
}

impl<T> Drop for IrqGuard<'_, T> {
    fn drop(&mut self) {
        self.mutex.inner.release();
        // SAFETY: `flags` was produced by the matching `munux_c_irq_save`
        // call on this same thread; restoring it is the documented
        // contract of the kernel entry point.
        unsafe { munux_c_irq_restore(self.flags) };
    }
}
