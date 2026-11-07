#include <iostream>
#include <vector>
#include <string>
#include <string_view>
#include <ranges>
#include <cmath>
#include <chrono>
#include <numeric>
#include <cctype>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <thread>
#include <oneapi/tbb.h>

// ============================================================================
// AUFGABE 1a: DAS SIEB DES ERATOSTHENES (n = 10^9 bis 4*10^9)
// ============================================================================

// 1. Классическое последовательное решето (строго 2 цикла for по книге, Abschn. 2.2.3)
size_t sieve_sequential(size_t n) {
    std::vector<uint8_t> is_prime(n + 1, 1);
    is_prime[0] = is_prime[1] = 0;

    // Цикл 1: базовые простые числа до sqrt(n)
    for (size_t p = 2; p * p <= n; ++p) {
        if (is_prime[p]) {
            // Цикл 2: вычеркивание составных чисел
            for (size_t i = p * p; i <= n; i += p) {
                is_prime[i] = 0;
            }
        }
    }

    // Подсчет через STL-алгоритм без явного цикла
    return std::count(is_prime.begin() + 2, is_prime.end(), 1);
}

// 2. Вспомогательная функция: получение базовых простых чисел до sqrt(n) для TBB
std::vector<size_t> get_base_primes(size_t limit) {
    std::vector<uint8_t> is_prime(limit + 1, 1);
    for (size_t p = 2; p * p <= limit; ++p) {
        if (is_prime[p]) {
            for (size_t i = p * p; i <= limit; i += p) {
                is_prime[i] = 0;
            }
        }
    }
    std::vector<size_t> primes;
    for (size_t p = 2; p <= limit; ++p) {
        if (is_prime[p]) primes.push_back(p);
    }
    return primes;
}

// 3. Параллельное блочное решето TBB (внутри задачи ровно 2 цикла for)
size_t sieve_parallel_tbb(size_t n, const std::vector<size_t>& base_primes, size_t block_size) {
    return tbb::parallel_reduce(
        tbb::blocked_range<size_t>(2, n + 1, block_size),
        size_t(0),
        [&](const tbb::blocked_range<size_t>& r, size_t local_count) -> size_t {
            size_t low = r.begin();
            size_t high = r.end() - 1;
            std::vector<uint8_t> block(high - low + 1, 1);

            // Цикл 1: обход базовых простых
            for (size_t p : base_primes) {
                if (p * p > high) break;
                size_t start = std::max(p * p, ((low + p - 1) / p) * p);
                // Цикл 2: вычеркивание в пределах L2-кэш блока
                for (size_t j = start; j <= high; j += p) {
                    block[j - low] = 0;
                }
            }

            // Подсчет без цикла for
            return local_count + std::count(block.begin(), block.end(), 1);
        },
        std::plus<size_t>()
    );
}

void run_aufgabe_1a() {
    std::cout << "========================================================\n";
    std::cout << "  AUFGABE 1a: SIEB DES ERATOSTHENES (MAX. OPTIMIERT)\n";
    std::cout << "========================================================\n";

    const size_t n = 4'000'000'000ULL;
    const size_t block_size = 256 * 1024; // 256 KB под L2-кэш

    // 1. Последовательный запуск (2 цикла for)
    std::cout << "1. Sequentielles Sieb (2 for-Schleifen, Buch Abschn. 2.2.3)...\n";
    auto t1 = std::chrono::high_resolution_clock::now();
    size_t seq_count = sieve_sequential(n);
    auto t2 = std::chrono::high_resolution_clock::now();
    auto seq_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    std::cout << "   -> Sequentiell: " << seq_count << " Primzahlen in " << seq_ms << " ms\n\n";

    // 2. Параллельный запуск TBB
    size_t sqrt_n = static_cast<size_t>(std::sqrt(n));
    auto base_primes = get_base_primes(sqrt_n);

    std::cout << "2. Paralleles TBB-Sieb (parallel_reduce)...\n";
    auto t3 = std::chrono::high_resolution_clock::now();
    size_t par_count = sieve_parallel_tbb(n, base_primes, block_size);
    auto t4 = std::chrono::high_resolution_clock::now();
    auto par_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();
    std::cout << "   -> Parallel TBB: " << par_count << " Primzahlen in " << par_ms << " ms\n\n";

    // Расчет метрик
    unsigned int threads = tbb::info::default_concurrency();
    if (threads == 0) threads = std::thread::hardware_concurrency();
    double speedup = (par_ms > 0) ? static_cast<double>(seq_ms) / par_ms : 1.0;
    double efficiency = (speedup / threads) * 100.0;

    std::cout << "Ergebnis Aufgabe 1a (n = " << n << "):\n";
    if (n == 1'000'000'000ULL) {
        std::cout << "  * Soll pi(10^9): 50847534 | Ist: " << par_count << "\n";
    } else if (n == 4'000'000'000ULL) {
        std::cout << "  * Soll pi(4*10^9): 189175540 | Ist: " << par_count << "\n";
    } else {
        std::cout << "  * Primzahlen Ist: " << par_count << "\n";
    }
    std::cout << "  * Threads:       " << threads << "\n";
    std::cout << "  * Speedup (S):   " << speedup << "x\n";
    std::cout << "  * Effizienz (E): " << efficiency << " %\n\n";
}

// ============================================================================
// AUFGABE 1b: LEXIKOGRAPHISCHE ANALYSE (OHNE EXPLIZITE SCHLEIFEN)
// ============================================================================

// Two-pass parallel_scan Body
class PrefixScanBody {
    const std::vector<uint8_t>& in;
    std::vector<uint32_t>& out;
    size_t sum = 0;
public:
    PrefixScanBody(const std::vector<uint8_t>& input, std::vector<uint32_t>& output)
        : in(input), out(output), sum(0) {}

    PrefixScanBody(PrefixScanBody& b, tbb::split)
        : in(b.in), out(b.out), sum(0) {}

    template <typename Tag>
    void operator()(const tbb::blocked_range<size_t>& r, Tag) {
        size_t temp = sum;
        for (size_t i = r.begin(); i < r.end(); ++i) {
            temp += in[i];
            if (Tag::is_final_scan()) out[i] = static_cast<uint32_t>(temp);
        }
        sum = temp;
    }

    void reverse_join(PrefixScanBody& a) { sum = a.sum + sum; }
    void assign(PrefixScanBody& b)       { sum = b.sum; }
    size_t get_sum() const               { return sum; }
};

// Параллельная токенизация через примитивы TBB (Map-Scan-Scatter)
std::vector<std::string_view> tokenize_parallel_tbb(const std::string& text) {
    size_t n = text.size();
    if (n == 0) return {};

    // 1. Map: флаг разделителя (uint8_t вместо int экономит 75% RAM)
    std::vector<uint8_t> is_delim(n);
    tbb::parallel_for(tbb::blocked_range<size_t>(0, n), [&](const auto& r) {
        for (size_t i = r.begin(); i < r.end(); ++i) {
            is_delim[i] = std::isspace(static_cast<unsigned char>(text[i])) ? 1 : 0;
        }
    });

    // 2. Scan: расчет индексов
    std::vector<uint32_t> prefix_sum(n);
    PrefixScanBody scan_body(is_delim, prefix_sum);
    tbb::parallel_scan(tbb::blocked_range<size_t>(0, n), scan_body);
    size_t total_delims = scan_body.get_sum();

    // 3. Scatter / Pack: запись позиций разделителей
    std::vector<size_t> delim_pos(total_delims + 2);
    delim_pos[0] = static_cast<size_t>(-1);
    delim_pos[total_delims + 1] = n;

    tbb::parallel_for(tbb::blocked_range<size_t>(0, n), [&](const auto& r) {
        for (size_t i = r.begin(); i < r.end(); ++i) {
            if (is_delim[i] == 1) delim_pos[prefix_sum[i]] = i;
        }
    });

    // Освобождаем промежуточные большие массивы перед шагом 4
    is_delim.clear();
    is_delim.shrink_to_fit();
    prefix_sum.clear();
    prefix_sum.shrink_to_fit();

    // 4. Map: параллельная сборка токенов через легковесный string_view (0 аллокаций в куче)
    std::vector<std::string_view> tokens(total_delims + 1);
    tbb::parallel_for(tbb::blocked_range<size_t>(0, total_delims + 1), [&](const auto& r) {
        for (size_t k = r.begin(); k < r.end(); ++k) {
            size_t start = delim_pos[k] + 1;
            size_t end = delim_pos[k + 1];
            if (end > start) {
                tokens[k] = std::string_view(text.data() + start, end - start);
            }
        }
    });

    return tokens;
}

// Последовательная версия: строго функциональная STL, БЕЗ явных циклов for
std::vector<std::string_view> tokenize_sequential(const std::string& text) {
    size_t n = text.size();
    if (n == 0) return {};

    // 1. Map via std::transform
    std::vector<uint8_t> is_delim(n);
    std::transform(text.begin(), text.end(), is_delim.begin(), [](char c) {
        return std::isspace(static_cast<unsigned char>(c)) ? uint8_t(1) : uint8_t(0);
    });

    // 2. Scan via std::inclusive_scan
    std::vector<uint32_t> prefix_sum(n);
    std::inclusive_scan(is_delim.begin(), is_delim.end(), prefix_sum.begin());
    uint32_t total_delims = prefix_sum.empty() ? 0 : prefix_sum.back();

    // 3. Scatter via std::views::iota и std::for_each (без создания вектора индексов в памяти!)
    std::vector<size_t> delim_pos(total_delims + 2);
    delim_pos[0] = static_cast<size_t>(-1);
    delim_pos[total_delims + 1] = n;

    auto indices = std::views::iota(size_t(0), n);
    std::for_each(indices.begin(), indices.end(), [&](size_t i) {
        if (is_delim[i] == 1) delim_pos[prefix_sum[i]] = i;
    });

    // Освобождаем промежуточные буферы перед созданием массива токенов
    is_delim.clear();
    is_delim.shrink_to_fit();
    prefix_sum.clear();
    prefix_sum.shrink_to_fit();

    // 4. Token-Extraktion via std::views::iota и std::for_each
    std::vector<std::string_view> tokens(total_delims + 1);
    auto token_indices = std::views::iota(size_t(0), static_cast<size_t>(total_delims + 1));
    std::for_each(token_indices.begin(), token_indices.end(), [&](size_t k) {
        size_t start = delim_pos[k] + 1;
        size_t end = delim_pos[k + 1];
        if (end > start) {
            tokens[k] = std::string_view(text.data() + start, end - start);
        }
    });

    return tokens;
}

void run_aufgabe_1b() {
    std::cout << "========================================================\n";
    std::cout << "  AUFGABE 1b: LEXIKOGRAPHISCHE ANALYSE (TBB PIPELINE)\n";
    std::cout << "========================================================\n";

    std::string text;
    std::ifstream file("input.txt");
    if (file.is_open()) {
        std::ostringstream ss;
        ss << file.rdbuf();
        text = ss.str();
        std::cout << "Datei 'input.txt' erfolgreich eingelesen.\n";
    } else {
        std::cout << "Keine 'input.txt' gefunden. Erzeuge synthetischen Textkorpus...\n";
        std::string sample = "Dies ist ein Karl May Textbeispiel fuer parallele Wortzerlegung mit Intel oneAPI TBB. ";
        for (int i = 0; i < 200'000; ++i) text += sample;
    }

    // Точное масштабирование до целевого размера без неконтролируемого удвоения до 7.3 ГБ
    const size_t target_size = 4'000'000'000ULL;
    if (text.size() < target_size) {
        text.reserve(target_size);
        while (text.size() * 2 <= target_size) {
            text += text;
        }
        if (text.size() < target_size) {
            text.append(text.data(), target_size - text.size());
        }
    }
    std::cout << "Verarbeitete Textgroesse: " << text.size() / (1024 * 1024) << " MB (" << text.size() << " Zeichen)\n\n";

    // 1. Последовательно (STL функционально, без for)
    std::cout << "1. Sequentielles Tokenizing (STL functional, ohne Schleifen)...\n";
    auto t1 = std::chrono::high_resolution_clock::now();
    auto tokens_seq = tokenize_sequential(text);
    auto t2 = std::chrono::high_resolution_clock::now();
    auto seq_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    std::cout << "   -> Sequentiell: " << tokens_seq.size() << " Tokens in " << seq_ms << " ms\n\n";

    // Освобождаем память после последовательного замера, чтобы не держать в RAM два гигантских массива токенов одновременно
    size_t seq_tokens_count = tokens_seq.size();
    tokens_seq.clear();
    tokens_seq.shrink_to_fit();

    // 2. Параллельно TBB
    std::cout << "2. Paralleles TBB-Tokenizing (Map-Scan-Scatter-Pipeline)...\n";
    auto t3 = std::chrono::high_resolution_clock::now();
    auto tokens_par = tokenize_parallel_tbb(text);
    auto t4 = std::chrono::high_resolution_clock::now();
    auto par_ms = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();
    std::cout << "   -> Parallel TBB: " << tokens_par.size() << " Tokens in " << par_ms << " ms\n\n";

    // 3. Reduktion (tbb::parallel_reduce): подсчет непустых слов
    size_t non_empty = tbb::parallel_reduce(
        tbb::blocked_range<size_t>(0, tokens_par.size()),
        size_t(0),
        [&](const tbb::blocked_range<size_t>& r, size_t sum) {
            for (size_t i = r.begin(); i < r.end(); ++i) {
                if (!tokens_par[i].empty()) sum++;
            }
            return sum;
        },
        std::plus<size_t>()
    );

    unsigned int threads = tbb::info::default_concurrency();
    if (threads == 0) threads = std::thread::hardware_concurrency();
    double speedup = (par_ms > 0) ? static_cast<double>(seq_ms) / par_ms : 1.0;
    double efficiency = (speedup / threads) * 100.0;

    std::cout << "Ergebnis Aufgabe 1b:\n";
    std::cout << "  * Tokens gesamt (Seq):  " << seq_tokens_count << "\n";
    std::cout << "  * Tokens gesamt (Par):  " << tokens_par.size() << "\n";
    std::cout << "  * Nicht-leere Tokens:   " << non_empty << "\n";
    std::cout << "  * Threads:              " << threads << "\n";
    std::cout << "  * Speedup (S):          " << speedup << "x\n";
    std::cout << "  * Effizienz (E):        " << efficiency << " %\n";
    std::cout << "========================================================\n";
}

int main() {
    std::cout << "Hochschule Osnabrueck - Parallele und Verteilte Algorithmen\n";
    std::cout << "Rechneruebung 1: TBB-Datenparallelitaet\n\n";

    run_aufgabe_1a();
    run_aufgabe_1b();

    return 0;
}
