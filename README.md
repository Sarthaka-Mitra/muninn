# Muninn

A relational database storage engine written in C++17 from scratch, built as a deep-dive into systems programming and compiler design.

---

## Overview

Muninn is not a wrapper around SQLite or a toy key-value store. It is a ground-up implementation of the core systems that power real relational databases — from binary file serialization to a disk-backed B-Tree index, to a recursive descent query parser that compiles WHERE clauses into Abstract Syntax Trees.

The name is taken from Norse mythology: Muninn is one of Odin's ravens, whose name means *Memory*. In the mythology, Muninn flies across the world each day and returns to Odin carrying everything it has observed — a fitting name for an engine whose entire purpose is to store, index, and retrieve information reliably.

---

## Architecture

Muninn is composed of five distinct systems:

```
┌──────────────────────────────────────────┐
│                   REPL                   │  src/main.cpp
├──────────────────────────────────────────┤
│              Query Parser                │  src/parser.cpp
│    Tokenizer → AST → Query Optimizer     │
├──────────────────────────────────────────┤
│               Database                   │  src/database.cpp
│         (Table registry / router)        │
├───────────────────┬──────────────────────┤
│       Table       │    Storage Engine    │  src/table.cpp
│  Schema + Rows    │  Binary file I/O     │  src/storage.cpp
├───────────────────┼──────────────────────┤
│    B-Tree Index   │       Pager          │  src/btree.cpp
│  Disk-backed tree │   4KB page manager   │  src/pager.cpp
└───────────────────┴──────────────────────┘
                     data/
              *.db (table rows)  *.idx (indexes)
```

### Component Responsibilities

| Component | Responsibility |
|---|---|
| `QueryParser` | Tokenizes raw input, builds an expression AST, and dispatches commands |
| `Database` | Maintains the table registry, routes queries to the correct table |
| `Table` | Owns the schema, validates rows, maintains active indexes, evaluates AST expressions |
| `StorageEngine` | Serializes and deserializes table rows to/from binary `.db` files |
| `BTreeIndex` | Manages a disk-backed B-Tree, owns a `Pager`, handles insert and search traversal |
| `BTreeNode` | Pure data container; serialized to/from 4KB page buffers via `memcpy` |
| `Pager` | Owns the `.idx` file descriptor; reads/writes fixed-size 4096-byte pages by offset |

---

## Key Technical Features

### 1. Disk-Backed B-Tree with Paging

Unlike a textbook B-Tree that lives entirely in RAM, Muninn's indexes persist across restarts. The `Pager` class divides each `.idx` file into fixed-size **4096-byte pages** (matching standard disk sector sizes). Each `BTreeNode` occupies exactly one page. Child pointers are not memory addresses — they are `uint32_t` page IDs.

When the tree needs to traverse to a child, it reads only that specific page from disk rather than loading the entire index into memory.

**Node page layout:**
```
Byte 0:        is_leaf (1 byte)
Bytes 1-2:     num_keys (uint16_t)
Bytes 3-15:    reserved
Bytes 16+:     Key entries (36 bytes each: 32-byte key string + 4-byte row index)
End of page:   Child page IDs (4 bytes each, uint32_t)
```

### 2. AST-Based Query Parser

`SELECT ... WHERE` clauses are compiled into an Abstract Syntax Tree using a **recursive descent parser** that respects standard operator precedence:

```
Expression    → AndExpr ( "OR" AndExpr )*
AndExpr       → Primary ( "AND" Primary )*
Primary       → col op val  |  "(" Expression ")"
```

Supported operators: `=`, `!=`, `>`, `>=`, `<`, `<=`

**Example AST** for `(age = 21 AND dept = CSE) OR age = 23`:
```
           [ OR ]
          /      \
      [ AND ]    [ age = 23 ]
      /     \
[age = 21] [dept = CSE]
```

### 3. Query Optimizer

Before evaluating an AST against rows, the engine inspects the tree for any equality condition (`=`) on an indexed column. If found, the engine uses the B-Tree to narrow the candidate row set before applying the full expression — reducing an O(N) full scan to O(log N + K) where K is the number of matching rows.

### 4. Binary Row Serialization

Table rows are stored to disk as compact binary files. Each field is preceded by an 8-byte size header:

```
[ 8-byte size ][ raw string bytes ][ 8-byte size ][ raw string bytes ] ...
```

Tables are automatically loaded from disk on startup and saved on every insert.

### 5. C++ Memory Safety

- **Move-only resource handles:** `BTreeIndex` and `Pager` own file streams and are move-only types (copy constructor/assignment deleted). This prevents double-close bugs and dangling file handles when stored inside `std::unordered_map`.
- **RAII throughout:** All destructors release resources deterministically.
- **Rule of Five applied** to both `BTreeIndex` and `Pager`.

---

## Supported Commands

```
CREATE <table> <col1> <type1> <col2> <type2> ...
INSERT <table> <val1> <val2> ...
SELECT <table>
SELECT <table> WHERE <expression>
CREATE INDEX <table> <column>
EXIT
```

**Expression examples:**
```
WHERE age = 21
WHERE age >= 21 AND dept = CSE
WHERE age = 21 AND name = Karthik
WHERE (age = 21 AND dept = CSE) OR age = 23
WHERE age != 21
```

Supported column types: `INT`, `TEXT`

---

## Building

**Prerequisites:** `g++` (C++17), `cmake >= 3.16`

```bash
git clone https://github.com/Sarthaka-Mitra/muninn.git
cd muninn
cmake -B build -S .
cmake --build build
cd build && ./muninn
```

---

## Project Structure

```
muninn/
├── include/
│   ├── types.h       # ColumnType, Row, Op enum, Expr AST node
│   ├── table.h       # Table class
│   ├── storage.h     # StorageEngine class
│   ├── database.h    # Database class
│   ├── parser.h      # QueryParser class
│   ├── btree.h       # BTreeNode, BTreeIndex classes
│   └── pager.h       # Pager class, PAGE_SIZE, PageBuffer
├── src/
│   ├── main.cpp      # REPL entry point
│   ├── table.cpp
│   ├── storage.cpp
│   ├── database.cpp
│   ├── parser.cpp
│   ├── btree.cpp
│   └── pager.cpp
└── data/             # Runtime data (gitignored)
    ├── *.db          # Binary table files
    └── *.idx         # Binary B-Tree index files
```

---

## What I Learned

This project was built as a deliberate exercise in systems-level C++ engineering after recognizing gaps in OOP design thinking. Key concepts applied and internalized:

- **Single Responsibility Principle:** Each class owns exactly one domain of knowledge (e.g., `Table` does not touch files; `Pager` knows nothing about B-Trees).
- **Binary I/O:** Translating C++ objects into flat byte sequences using `reinterpret_cast`, `memcpy`, and fixed byte offsets.
- **Compiler Design:** Implementing a tokenizer, recursive descent parser, and AST evaluator from first principles.
- **Memory Ownership:** Applying the Rule of Five to move-only resource handles, resolving dangling pointer bugs caused by `std::unordered_map` rehashing.
- **Disk vs RAM:** Understanding why memory pointers cannot persist across program restarts and how real databases substitute them with stable page IDs.
