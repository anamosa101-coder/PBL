# Page Replacement Algorithms Simulator
> **Course:** Operating Systems — Project-Based Learning (PBL)  
> **Topic:** Virtual Memory Management & Page Replacement Policies  
> **Team Member Assigned:** `TBD`  
> **Implementation:** Pure Client-Side Web Application (HTML5, Vanilla CSS3, Modern JavaScript)

---

## Table of Contents
1. [Introduction to Virtual Memory & Page Replacement](#1-introduction-to-virtual-memory--page-replacement)
2. [Algorithms Implemented](#2-algorithms-implemented)
   - [FIFO (First-In, First-Out)](#fifo-first-in-first-out)
   - [LRU (Least Recently Used)](#lru-least-recently-used)
   - [Optimal Page Replacement (Bélády's OPT)](#optimal-page-replacement-béládys-opt)
3. [Bélády's Anomaly Deep Dive](#3-béládys-anomaly-deep-dive)
4. [Step-by-Step Example Walkthrough](#4-step-by-step-example-walkthrough)
   - [FIFO Step Trace (9 Faults)](#fifo-step-trace-reference-7-0-1-2-0-3-0-4-2-3-frames--3)
   - [Comparative Analysis (FIFO vs LRU vs Optimal)](#comparative-analysis-on-example-string)
5. [Application Features & User Guide](#5-application-features--user-guide)
6. [How to Run](#6-how-to-run)
7. [Viva & Oral Defense Questions (FAQ)](#7-viva--oral-defense-questions-faq)

---

## 1. Introduction to Virtual Memory & Page Replacement

### Virtual Memory Overview
In modern computer architectures, physical memory (RAM) is finite. **Virtual Memory** is a memory management capability of an operating system (OS) that uses hardware (Memory Management Unit or MMU) and secondary storage (SSD/HDD swap space) to provide applications with the illusion of an expansive, contiguous address space.

- **Pages**: Virtual address space is divided into fixed-size logical units called **pages** (commonly 4 KB).
- **Frames**: Physical RAM is divided into matching fixed-size blocks called **page frames**.
- **Page Table**: A mapping structure maintained by the OS for each process, tracking which virtual pages reside in which physical frames. A **Valid/Invalid Bit** indicates if the page is currently in RAM (`1`) or on disk (`0`).

```
Virtual Memory (Pages)          Page Table             Physical RAM (Frames)
┌──────────────┐             ┌──────┬───────┐            ┌──────────────┐
│ Page 0       │────────────>│ P# 0 │ Fr 2  │───────────>│ Frame 0: P2  │
├──────────────┤             ├──────┼───────┤            ├──────────────┤
│ Page 1       │────────────>│ P# 1 │ Inval │──[Disk]    │ Frame 1: P3  │
├──────────────┤             ├──────┼───────┤            ├──────────────┤
│ Page 2       │────────────>│ P# 2 │ Fr 0  │───────────>│ Frame 2: P0  │
├──────────────┤             ├──────┼───────┤            └──────────────┘
│ Page 3       │────────────>│ P# 3 │ Fr 1  │
└──────────────┘             └──────┴───────┘
```

### What is a Page Fault?
When a CPU instruction references a virtual page whose valid/invalid bit is marked **Invalid** (not in physical RAM), the MMU triggers a hardware interrupt known as a **Page Fault**:
1. The CPU traps into OS kernel mode.
2. The OS suspends the running process.
3. The OS locates the requested page in secondary backing storage (swap partition/file).
4. A free physical frame is sought in RAM.
5. If **all frames are occupied**, the OS must select an existing resident page to evict to disk. This eviction decision is governed by a **Page Replacement Algorithm**.
6. The requested page is read from disk into the freed frame.
7. The Page Table is updated (valid bit set to `1`).
8. The trapped instruction is restarted seamlessly.

### Why Page Replacement Efficiency Matters
Disk I/O latency is orders of magnitude slower than semiconductor RAM (nanoseconds vs milliseconds). A high page fault rate induces **thrashing**, where the CPU spends more time swapping pages in and out than executing useful instructions. Minimizing page faults via intelligent page replacement algorithms is paramount to operating system throughput.

---

## 2. Algorithms Implemented

### FIFO (First-In, First-Out)

#### Concept
The simplest page replacement algorithm. The operating system associates with each page the timestamp when it entered memory, or maintains a FIFO queue of loaded frames. When replacement is required, the page at the head of the queue (the oldest resident page) is selected for eviction.

```
Incoming: [ 2 ]
Queue: [ 7 ] -> [ 0 ] -> [ 1 ]   ==> Evicts [ 7 ] (Head of Queue)
New Queue: [ 0 ] -> [ 1 ] -> [ 2 ]
```

#### Advantages
- **Simplicity**: Trivial to implement using a circular queue or array index pointer.
- **Low Overhead**: Does not require hardware support, timers, or tracking on every memory access.

#### Disadvantages
- **Ignores Usage Frequency and Recency**: An essential, heavily accessed utility library page (e.g., standard C runtime) may be evicted simply because it was loaded early during process initialization.
- **Suffers from Bélády's Anomaly**: Increasing physical frame allocation can paradoxically *increase* page faults.

---

### LRU (Least Recently Used)

#### Concept
Approximates the Optimal algorithm by exploiting the **Principle of Temporal Locality**: *pages accessed recently are highly likely to be accessed again in the near future*. When a page fault occurs and frames are full, LRU evicts the page that has not been accessed for the longest period of time.

```
Time:      t0    t1    t2    t3    t4    t5
Page:       7     0     1     2     0     3
LRU Order:             (1)   (0)   (2)   ==> 1 was least recently accessed at t2
Frame update: [2, 0, 1] -> Evict 1 -> [2, 0, 3]
```

#### Implementation Mechanisms
- **Hardware Counter / Timestamp**: Every page table entry includes a "Time-of-Use" field. A global clock/counter ticks on every memory reference, copying the counter into the referenced page's entry. On fault, the frame with the smallest counter value is evicted.
- **Stack / Doubly-Linked List**: Whenever a page is referenced, it is detached from its position and moved to the top of the stack. The bottom of the stack always holds the least recently used page.

#### Advantages
- **High Hit Rates**: Strongly aligns with real-world program locality of reference.
- **Immune to Bélády's Anomaly**: Belongs to the mathematical class of **Stack Algorithms**.

#### Disadvantages
- **High Hardware Overhead**: Updating timestamps or linked-list pointers on *every single memory instruction* without slowing down memory cycle time requires dedicated hardware support.
- Pure LRU is often too expensive for production OS kernels, leading to approximations such as the **Clock (Second-Chance) Algorithm**.

---

### Optimal Page Replacement (Bélády's OPT)

#### Concept
Proposed by László Bélády in 1966. Evicts the page that **will not be used for the longest period of time in the future**. If a page is never referenced again, its distance to next reference is $\infty$.

```
Current Memory: { 2, 0, 1 }
Future References: [ 0, 3, 0, 4, 2, 3 ]
Next use of 2: 4 steps later
Next use of 0: 1 step later
Next use of 1: NEVER (Infinity)
Decision: Evict 1 (Longest future distance)
```

#### Advantages
- **Theoretical Benchmark**: Mathematically proven to produce the **absolute minimum number of page faults** for any fixed reference string and frame count.
- **Zero Anomaly**: Like LRU, it is a stack algorithm and is immune to Bélády's Anomaly.

#### Disadvantages
- **Unrealizable in Real-Time OS**: Requires complete clairvoyant knowledge of future memory access patterns, which an online operating system cannot predict in general-purpose computing.
- Primarily used in laboratory analysis and simulation to benchmark the performance of practical replacement policies.

---

## 3. Bélády's Anomaly Deep Dive

### Definition
In 1969, László Bélády discovered an unexpected paradox in FIFO page replacement:  
**Increasing the number of physical page frames allocated to a process can result in an INCREASE in the total number of page faults.**

### Why does it occur?
For an algorithm to guarantee that more frames never increase page faults, it must satisfy the **Inclusion Property**:
$$\mathcal{M}(m, r) \subseteq \mathcal{M}(m + 1, r)$$
*The set of pages residing in memory with $m$ frames at reference $r$ must always be a subset of the pages residing in memory with $m+1$ frames.*

- **Stack Algorithms** (e.g., LRU, Optimal, LFU) strictly satisfy the Inclusion Property because memory contents represent the top $m$ elements of an ordered preference stack.
- **FIFO does NOT satisfy the Inclusion Property**. The eviction decision in FIFO depends solely on *when the page was loaded*, not on the intrinsic access pattern. Increasing frame capacity shifts the replacement order in unpredictable ways.

### Concrete Mathematical Proof & Example
Consider the reference string:
$$\mathbf{1,\; 2,\; 3,\; 4,\; 1,\; 2,\; 5,\; 1,\; 2,\; 3,\; 4,\; 5}$$

| Step | Reference | FIFO with 3 Frames | FIFO with 4 Frames |
| :---: | :---: | :--- | :--- |
| 1 | `1` | [1, -, -] *(Fault 1)* | [1, -, -, -] *(Fault 1)* |
| 2 | `2` | [1, 2, -] *(Fault 2)* | [1, 2, -, -] *(Fault 2)* |
| 3 | `3` | [1, 2, 3] *(Fault 3)* | [1, 2, 3, -] *(Fault 3)* |
| 4 | `4` | [4, 2, 3] *(Fault 4)* | [1, 2, 3, 4] *(Fault 4)* |
| 5 | `1` | [4, 1, 3] *(Fault 5)* | [1, 2, 3, 4] **(HIT 1)** |
| 6 | `2` | [4, 1, 2] *(Fault 6)* | [1, 2, 3, 4] **(HIT 2)** |
| 7 | `5` | [5, 1, 2] *(Fault 7)* | [5, 2, 3, 4] *(Fault 5)* |
| 8 | `1` | [5, 1, 2] **(HIT 1)** | [5, 1, 3, 4] *(Fault 6)* |
| 9 | `2` | [5, 1, 2] **(HIT 2)** | [5, 1, 2, 4] *(Fault 7)* |
| 10 | `3` | [3, 1, 2] *(Fault 8)* | [5, 1, 2, 3] *(Fault 8)* |
| 11 | `4` | [3, 4, 2] *(Fault 9)* | [4, 1, 2, 3] *(Fault 9)* |
| 12 | `5` | [3, 4, 5] *(Fault 10 - wait! with 3 frames, total faults = 9)* | [4, 5, 2, 3] *(Fault 10)* |

- **3 Frames:** **9 Page Faults**
- **4 Frames:** **10 Page Faults**

Increasing RAM by 33% resulted in **11.1% more page faults**!  
*(You can test this interactively in the simulator with the ⚡ Belady's Anomaly preset).*

---

## 4. Step-by-Step Example Walkthrough

### Problem Statement
- **Page Reference String:** `7 0 1 2 0 3 0 4 2 3` (Length = 10)
- **Available Frame Slots:** `3`

---

### FIFO Step Trace (Reference: `7 0 1 2 0 3 0 4 2 3`, Frames = 3)

| Step | Page Ref | Frame 0 | Frame 1 | Frame 2 | Status | Evicted | FIFO Queue Order (Oldest -> Newest) | Running Faults | Running Hits |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|:---|:---:|:---:|
| **1** | `7` | **7** | - | - | <span style="color:red">**FAULT**</span> | - | [7] | 1 | 0 |
| **2** | `0` | 7 | **0** | - | <span style="color:red">**FAULT**</span> | - | [7, 0] | 2 | 0 |
| **3** | `1` | 7 | 0 | **1** | <span style="color:red">**FAULT**</span> | - | [7, 0, 1] | 3 | 0 |
| **4** | `2` | **2** | 0 | 1 | <span style="color:red">**FAULT**</span> | 7 | [0, 1, 2] *(7 evicted)* | 4 | 0 |
| **5** | `0` | 2 | 0 | 1 | <span style="color:green">**HIT**</span> | - | [0, 1, 2] *(queue unchanged)* | 4 | 1 |
| **6** | `3` | 2 | **3** | 1 | <span style="color:red">**FAULT**</span> | 0 | [1, 2, 3] *(0 evicted)* | 5 | 1 |
| **7** | `0` | 2 | 3 | **0** | <span style="color:red">**FAULT**</span> | 1 | [2, 3, 0] *(1 evicted)* | 6 | 1 |
| **8** | `4` | **4** | 3 | 0 | <span style="color:red">**FAULT**</span> | 2 | [3, 0, 4] *(2 evicted)* | 7 | 1 |
| **9** | `2` | 4 | **2** | 0 | <span style="color:red">**FAULT**</span> | 3 | [0, 4, 2] *(3 evicted)* | 8 | 1 |
| **10** | `3` | 4 | 2 | **3** | <span style="color:red">**FAULT**</span> | 0 | [4, 2, 3] *(0 evicted)* | **9** | **1** |

#### FIFO Results
- **Total References:** 10
- **Total Page Faults:** **9** (90.0% Fault Ratio)
- **Total Page Hits:** **1** (10.0% Hit Ratio)

---

### Comparative Analysis on Example String

| Metric | FIFO | LRU | Optimal (OPT) |
|:---|:---:|:---:|:---:|
| **Total References** | 10 | 10 | 10 |
| **Page Faults** | **9** | **8** | **6** *(Best)* |
| **Page Hits** | 1 | 2 | 4 |
| **Hit Ratio (%)** | 10.0% | 20.0% | **40.0%** |
| **Fault Ratio (%)** | 90.0% | 80.0% | **60.0%** |
| **Efficiency vs Optimal** | +3 Faults | +2 Faults | Optimal Baseline |

- **Why LRU performed better (8 faults):** LRU kept page `0` in memory when page `3` arrived because page `0` had just been hit at step 5. Consequently, step 7 was a Hit under LRU!
- **Why Optimal achieved 6 faults:** At step 4 (page `2`), Optimal saw that `7` and `1` were never needed again, evicting `7`. At step 6 (page `3`), it evicted `1` (never needed again), retaining `2` and `0` for future hits.

---

## 5. Application Features & User Guide

### 1. Control Panel
- **Page Reference Input**: Space or comma-delimited list of integers (e.g., `7 0 1 2 0 3 0 4 2 3`).
- **Memory Frame Count**: Configurable frame allocation between 1 and 8 slots.
- **Preset Quick-Fill Buttons**:
  - `Standard Example`: Fills the syllabus example (`7 0 1 2 0 3 0 4 2 3`, 3 frames).
  - `Belady's Anomaly`: Fills `1 2 3 4 1 2 5 1 2 3 4 5`.
  - `High Locality`: Fills repeated looping references (`1 2 3 2 1 5 2 1 6 2...`).
  - `Random String`: Generates an arbitrary valid reference sequence.
  - `Clear`: Resets all fields.

### 2. Interactive Stepper & HUD
- **Step Back (◀) & Step Forward (▶)**: Inspect memory state transformation step-by-step.
- **Auto Play (▶ / ⏸)**: Hands-free automatic animation through references at variable speeds (0.5x, 1x, 2x, 3.5x).
- **Scrubber Slider**: Drag smoothly across the timeline to inspect any step instantly.
- **Live Explanation Engine**: Every step features human-readable narrative explaining whether a hit or fault occurred, which page was evicted, and why according to the active algorithm's policy.

### 3. Dynamic Memory Matrix Visualization
- Columns represent each reference step; rows represent physical frame slots.
- Incoming **Faults** are highlighted with glowing crimson badges and borders.
- Incoming **Hits** are highlighted with electric teal halos.
- An **Evicted Page Row** explicitly documents every victim page removed from physical memory.

### 4. Performance Summary Dashboard
- High-contrast stat cards for Total References, Faults, Hits, and Hit Ratio.
- Visual animated gradient progress bars for Hit Ratio and Fault Ratio.
- Mathematical formula references displayed right under meters.

### 5. Compare All Mode
- Executes FIFO, LRU, and Optimal simultaneously on the identical reference sequence and frame budget.
- Side-by-side benchmark summary table with performance rankings.
- Gold crown and badge (`★ Winner`) awarded to the top-performing policy.
- **HTML5 Canvas Bar Chart**: Custom-drawn, dependency-free bar chart comparing faults vs hits across all algorithms with high-DPI retina sharpness.
- **Belady's Anomaly Lab**: One-click proof comparing FIFO at 3 vs 4 frames.

---

## 6. How to Run

The simulator is built entirely with vanilla web standards and requires **no build tools, node packages, or server runtimes**.

### Option A: Direct Browser Launch
1. Navigate to the project directory:
   ```
   C:\Users\LENOVO\OneDrive\Desktop\PBL\PBL\pagereplace\
   ```
2. Double-click `index.html` (or right-click → **Open with** → Google Chrome, Microsoft Edge, Firefox, or Brave).
3. The simulator will load instantly with the standard PBL example.

### Option B: Local Web Server (Optional)
If running inside VS Code:
```powershell
# Using Python built-in HTTP server:
cd "C:\Users\LENOVO\OneDrive\Desktop\PBL\PBL\pagereplace"
python -m http.server 8080
# Open http://localhost:8080 in browser
```

---

## 7. Viva & Oral Defense Questions (FAQ)

### Q1: What is the primary difference between FIFO and LRU?
> **Answer:** FIFO bases eviction decisions purely on **arrival time** (chronological order of loading into memory), completely ignoring whether the page is still actively used. LRU bases eviction on **recency of reference** (access history), evicting the page that has not been referenced for the longest time in the past.

### Q2: Why is the Optimal page replacement algorithm called unrealizable?
> **Answer:** Optimal replacement requires knowing the future sequence of page requests in advance. In a real-time, multitasking operating system, memory references depend on user interaction, branching logic, I/O interrupts, and external conditions, making deterministic foreknowledge impossible.

### Q3: What is a Stack Algorithm and why does it prevent Bélády's Anomaly?
> **Answer:** An algorithm is a stack algorithm if the set of pages in memory for $n$ frames is always a subset of the pages in memory for $n+1$ frames ($\mathcal{M}(n) \subseteq \mathcal{M}(n+1)$). Because adding a frame can only add pages to memory without removing any existing ones, the hit count can never decrease. LRU and Optimal are stack algorithms; FIFO is not.

### Q4: How does Linux or Windows approximate LRU in production?
> **Answer:** Pure LRU requires high hardware overhead on every memory cycle. Modern OS kernels use the **Second-Chance (Clock) Algorithm** or a **Two-Handed Clock Algorithm**. Each page has a reference bit set to `1` by hardware on read/write. The clock hand sweeps frames: if the bit is `1`, it is cleared to `0` and given a second chance; if the bit is `0`, the page is evicted.

---

*Operating Systems Project-Based Learning (PBL) — Built for Academic Excellence.*
