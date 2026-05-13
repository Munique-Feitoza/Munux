#![no_std]
#![forbid(unsafe_op_in_unsafe_fn)]
#![deny(clippy::undocumented_unsafe_blocks)]
#![deny(clippy::missing_safety_doc)]
#![warn(clippy::pedantic)]
#![allow(clippy::module_name_repetitions)]

pub mod heap;
pub mod sync;
