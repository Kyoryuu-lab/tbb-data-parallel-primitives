<h1>tbb-data-parallel-primitives</h1>
<p>Data-parallel algorithms with Intel oneAPI TBB: Segmented Sieve of Eratosthenes and Lexical Text Tokenizer</p>

<h2>Project description</h2>
<p>This project implements high-performance data-parallel patterns in modern C++20 using oneAPI Threading Building Blocks (TBB):</p>
<ol>
  <li>
    <dl>
      <dt><b>Aufgabe 1a — Sieve of Eratosthenes (n = 10⁹):</b></dt>
      <dd>Computes all prime numbers up to <code>1,000,000,000</code>.</dd>
      <dd>Partitions the search range into 256 KB L2-cache-friendly segments to avoid bus saturation.</dd>
      <dd>Calculates prime count via <code>tbb::parallel_reduce</code> and verifies the exact value: <b>π(10⁹) = 50,847,534</b>.</dd>
    </dl>
  </li>
  <li>
    <dl>
      <dt><b>Aufgabe 1b — Lexicographical Analysis (Tokenization):</b></dt>
      <dd>Converts unstructured raw text (<code>std::string</code>) into <code>std::vector&lt;std::string&gt;</code> tokens.</dd>
      <dd>Eliminates explicit loops by applying a Map-Scan-Scatter-Reduce skeleton pipeline.</dd>
      <dd>Uses <code>tbb::parallel_for</code> (Map), <code>tbb::parallel_scan</code> (Prefix Sum), and parallel extraction.</dd>
    </dl>
  </li>
  <li>Measures execution time without I/O and calculates parallel Speedup (S) and Efficiency (E).</li>
</ol>

<h2>Project files</h2>
<table>
  <tr>
    <th colspan="2">tbb-data-parallel-primitives</th>
  </tr>
  <tr>
    <td>CMakeLists.txt</td>
    <td>Build setup linking <code>TBB::tbb</code> (C++20)</td>
  </tr>
  <tr>
    <td>main.cpp</td>
    <td>Integrated benchmark executable executing both parallel tasks</td>
  </tr>
  <tr>
    <td>README.md</td>
    <td>Project documentation and benchmark evaluation</td>
  </tr>
</table>

<h2>Installation and launch</h2>
<ol>
  <b>
    <li>
      <dl>
        <dt>Clone the repository</dt>
        <dd>
          <code>git clone https://github.com/Kyoryuu-lab/tbb-data-parallel-primitives.git</code>
        </dd>
        <dd>
          <code>cd tbb-data-parallel-primitives</code>
        </dd>
      </dl>
    </li>
  </b>
  <b>
    <li>
      <dl>
        <dt>Install dependencies (macOS / Linux)</dt>
        <dd>
          <code>brew install tbb</code> <i>(macOS)</i> or <code>sudo apt install libtbb-dev</code> <i>(Ubuntu/Debian)</i>
        </dd>
      </dl>
    </li>
  </b>
  <b>
    <li>
      <dl>
        <dt>Build and execute</dt>
        <dd>
          <code>cmake -B build -DCMAKE_BUILD_TYPE=Release</code>
        </dd>
        <dd>
          <code>cmake --build build --config Release</code>
        </dd>
        <dd>
          <code>./build/main</code>
        </dd>
      </dl>
    </li>
  </b>
</ol>

<h2>Example of console output</h2>
<samp>
  <p>
    ========================================================================<br />
    PVA Rechneruebung 1: TBB Parallel Algorithms<br />
    ========================================================================<br />
    [Aufgabe 1a] Sieve of Eratosthenes (n = 1000000000)<br />
    [Sequential] Found: 50847534 primes in 5840.12 ms<br />
    [Parallel TBB] Found: 50847534 primes in 615.34 ms<br />
    Speedup: 9.50x | Efficiency: 59.4 %<br />
    <br />
    [Aufgabe 1b] Parallel Lexicographical Analysis (Text size: 45.2 MB)<br />
    Tokens generated: 7154816<br />
    [Sequential] Time: 1845.20 ms<br />
    [Parallel TBB] Time: 310.15 ms<br />
    Speedup: 5.95x | Efficiency: 37.2 %<br />
    ========================================================================
  </p>
</samp>
