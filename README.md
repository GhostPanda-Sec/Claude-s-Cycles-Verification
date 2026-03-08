[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

## Introduction / 简介

This project provides a pure C implementation to verify the construction discovered by Claude Opus 4.6 (as described in Donald Knuth's recent paper *"Claude's Cycles"*, revised March 4, 2026). For any odd integer **m > 2**, the construction partitions all arcs of a certain directed graph into three Hamiltonian cycles. The code validates that each cycle indeed visits every vertex exactly once (Hamiltonian property) and that the three cycles together use every outgoing arc exactly once (arc coverage).

本项目提供了一个纯 C 语言实现，用于验证 Claude Opus 4.6 发现的构造（如高德纳最新论文 *《Claude's Cycles》* 2026年3月4日修订版所述）。对于任意奇数 **m > 2**，该构造将某个有向图的所有弧划分为三个哈密顿环。代码验证每个环确实恰好访问每个顶点一次（哈密顿性），且三个环共同使用每条出弧恰好一次（弧覆盖）。

---

## Paper Summary / 论文解读

### Problem / 问题

Consider the digraph with vertices \((i, j, k)\) where \(0 \le i, j, k < m\). From each vertex there are three outgoing arcs:

- to \(((i+1)\bmod m,\; j,\; k)\)
- to \((i,\; (j+1)\bmod m,\; k)\)
- to \((i,\; j,\; (k+1)\bmod m)\)

The goal is to partition all \(3m^3\) arcs into three directed cycles, each visiting every vertex exactly once (i.e., Hamiltonian cycles), with no two cycles sharing an arc.

### Claude's Discovery / Claude 的发现

Claude Opus 4.6 found a general construction for all odd \(m>2\). The rules are given in terms of \(s = (i+j+k)\bmod m\) and the current vertex. Three cycles (numbered 0,1,2) are defined by the following bump rules ("bump x" means increase x by 1 modulo m):

**Cycle 0** (from page 3 of the paper):
- If \(s = 0\):
  - if \(j = m-1\), bump \(i\);
  - else bump \(k\).
- If \(0 < s < m-1\):
  - if \(i = m-1\), bump \(k\);
  - else bump \(j\).
- If \(s = m-1\):
  - if \(i > 0\), bump \(j\);
  - else bump \(k\).

**Cycle 1** (from the appendix):
- If \(s = 0\): bump \(j\).
- If \(0 < s < m-1\): bump \(i\).
- If \(s = m-1\):
  - if \(i > 0\), bump \(k\);
  - else bump \(j\).

**Cycle 2** (from the appendix):
- If \(s = 0\):
  - if \(j < m-1\), bump \(i\);
  - else bump \(k\).
- If \(0 < s < m-1\):
  - if \(i < m-1\), bump \(k\);
  - else bump \(j\).
- If \(s = m-1\): bump \(i\).

The paper proves that these rules indeed yield three disjoint Hamiltonian cycles for any odd \(m>2\). This code provides an empirical verification of that theorem.

---

## Algorithm Overview / 原理大纲

The verification program follows these steps:

1. **Vertex encoding**: Each vertex \((i,j,k)\) is mapped to a single index \(idx = i\cdot m^2 + j\cdot m + k\) for fast array access.
2. **Precomputation**: For each cycle \(c=0,1,2\), an array `next[c][idx]` is computed storing the index of the successor of vertex `idx` according to the bump rules above.
3. **Hamiltonian check**: Starting from vertex 0, traverse the cycle for \(m^3\) steps. Using a bitmap (1 bit per vertex) we ensure every vertex is visited exactly once and that we return to the start.
4. **Arc coverage check**: For every vertex, compute its three natural outgoing targets \(t_0,t_1,t_2\) (the three possible bumps). Then verify that the set \(\{ \text{next}_0[v], \text{next}_1[v], \text{next}_2[v] \}\) equals \(\{ t_0, t_1, t_2 \}\) and that all three values are distinct (i.e., each cycle uses a different outgoing arc).

验证程序遵循以下步骤：

1. **顶点编码**：每个顶点 \((i,j,k)\) 映射为单一索引 \(idx = i\cdot m^2 + j\cdot m + k\)，便于快速数组访问。
2. **预计算**：对于每个环 \(c=0,1,2\)，计算数组 `next[c][idx]`，存储顶点 `idx` 根据上述 bump 规则的后继索引。
3. **哈密顿性检查**：从顶点 0 出发，沿着环走 \(m^3\) 步。使用位图（每个顶点 1 比特）确保每个顶点恰好访问一次，并且最后回到起点。
4. **弧覆盖检查**：对于每个顶点，计算其三条自然出弧的目标 \(t_0,t_1,t_2\)（即三个可能的 bump）。然后验证集合 \(\{ \text{next}_0[v], \text{next}_1[v], \text{next}_2[v] \}\) 等于 \(\{ t_0, t_1, t_2 \}\)，且三个值互异（即每个环使用不同的出弧）。

---

## Program Usage / 脚本用途

The program `verify` (or `verify.exe` on Windows) accepts one optional command-line argument: an odd integer \(m>2\).  
- If an argument is provided, it runs the verification only for that \(m\).  
- If no argument is given, it tests the default values \(m = 3,5,7,9\).

For each \(m\) it prints:
- The number of vertices (\(m^3\)).
- Time to build the successor tables.
- Pass/fail status for each of the three cycles (Hamiltonian check).
- Time for Hamiltonian verification.
- Time for arc coverage verification.
- Final verdict (ALL TESTS PASSED or TESTS FAILED).

程序 `verify`（Windows 下为 `verify.exe`）接受一个可选的命令行参数：奇数 \(m>2\)。
- 如果提供参数，则只对该 \(m\) 进行验证。
- 如果不提供参数，则测试默认值 \(m = 3,5,7,9\)。

对于每个 \(m\)，程序输出：
- 顶点数 (\(m^3\))
- 构建后继表的时间
- 三个环各自的通过/失败状态（哈密顿性检查）
- 哈密顿验证总时间
- 弧覆盖验证时间
- 最终结论（全部通过或测试失败）

---

## Compilation / 编译

The code is written in standard C (C89-compatible) and has **no external dependencies**. Any C compiler (GCC, Clang, MSVC, etc.) can build it.  

The basic compilation command using GCC is:

```bash
gcc -O3 verify_cycles.c -o verify
```

On Windows this produces verify.exe; on Linux/macOS it produces verify.

代码使用标准 C 编写（兼容 C89），无任何外部依赖。任何 C 编译器（GCC、Clang、MSVC 等）均可编译。

使用 GCC 的基础编译命令为：

```bash
gcc -O3 verify_cycles.c -o verify
```

在 Windows 上生成 verify.exe，在 Linux/macOS 上生成 verify。

## Execution Examples / 运行示例

```bash
# Test with m=11
./verify 11

# Test the default set
./verify
```

Example output for m=11:

```text
Claude's Cycles Verification
====================================================================

=== Verifying m = 11 ===
Graph: 11^3 = 1331 vertices
Build time: 0.0000 seconds
Cycle 0: PASSED (Hamiltonian)
Cycle 1: PASSED (Hamiltonian)
Cycle 2: PASSED (Hamiltonian)
Hamiltonian verification time: 0.0000 seconds
Arc coverage verification time: 0.0000 seconds

*** ALL TESTS PASSED for m = 11 ***

====================================================================
Total execution time: 0.0040 seconds
```

## Memory Considerations / 内存注意事项

For a given \(m\), the program allocates three next arrays, each of size \(m^3\) uint32_t entries (4 bytes each), plus a bitmap of \((m^3 +7)/8\) bytes. Total memory ≈ \(3 \cdot m^3 \cdot 4 + m^3 /8\) bytes.

Examples:
- \(m=111\) → ~1367631 vertices → ~16.4 MB + 0.17 MB = ~16.6 MB
- \(m=201\) → ~8120601 vertices → ~97.4 MB + 1.0 MB = ~98.4 MB
- \(m=401\) → ~64481201 vertices → ~774 MB + 7.7 MB = ~782 MB
- \(m=1111\) → ~1.37e9 vertices → ~16.4 GB + 0.17 GB = ~16.6 GB

Make sure your system has enough free memory before running with large \(m\). For extremely large \(m\), consider using a machine with sufficient RAM or reduce \(m\) to a manageable size.

对于给定的 \(m\)，程序分配三个 next 数组，每个大小为 \(m^3\) 个 uint32_t 条目（每个 4 字节），外加一个 \((m^3 +7)/8\) 字节的位图。总内存 ≈ \(3 \cdot m^3 \cdot 4 + m^3 /8\) 字节。

示例：
- \(m=111\) → 约 1367631 顶点 → 约 16.4 MB + 0.17 MB = 16.6 MB
- \(m=201\) → 约 8120601 顶点 → 约 97.4 MB + 1.0 MB = 98.4 MB
- \(m=401\) → 约 64481201 顶点 → 约 774 MB + 7.7 MB = 782 MB
- \(m=1111\) → 约 13.7 亿顶点 → 约 16.4 GB + 0.17 GB = 16.6 GB

在运行大 \(m\) 之前，请确保系统有足够空闲内存。对于极大 \(m\)，请考虑使用具有足够 RAM 的机器，或将 \(m\) 减小到可管理的范围。

---

## License / 许可证

This project is licensed under the MIT License – see the LICENSE file for details.

本项目采用 MIT 许可证，详情请参阅 LICENSE 文件。