// Copyright RIME Developers
// Distributed under the BSD License
//
// Rime input method engine, ported to Rust.
//
// This crate mirrors the layered architecture of the original C++ librime:
//   key_table/key_event — key model
//   candidate/translation — candidate stream abstraction
//   segmentation/composition/menu — segment & menu model
//   context/commit_history — per-session state
//   filter/processor/segmentor/translator/formatter — pipeline component traits

pub mod algo;
pub mod candidate;
pub mod commit_history;
pub mod component;
pub mod composition;
pub mod config;
pub mod context;
pub mod dict;
pub mod engine;
pub mod filter;
pub mod formatter;
pub mod gear;
pub mod key_event;
pub mod key_table;
pub mod menu;
pub mod processor;
pub mod schema;
pub mod segmentation;
pub mod segmentor;
pub mod service;
pub mod switches;
pub mod ticket;
pub mod translation;
pub mod translator;
pub mod util;
pub mod yaml;
