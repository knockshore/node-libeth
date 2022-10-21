// JS wrapper for using libethash native functions
// The code mostly stripped from cpp-etchash

#include <stdint.h>

#include <iomanip>
#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <type_traits>

#include <nan.h>

constexpr static int light_cache_init_size = 1 << 24;
constexpr static int light_cache_growth = 1 << 17;
constexpr static int light_cache_rounds = 3;
constexpr static int full_dataset_init_size = 1 << 30;
constexpr static int full_dataset_growth = 1 << 23;
constexpr static int full_dataset_item_parents = 256;
constexpr static int ecip_1099_activation_epoch = 390; // classic mainnet
constexpr size_t l1_cache_size = 16 * 1024;
static const uint32_t fnv_prime = 0x01000193;

#define ETHASH_LIGHT_CACHE_ITEM_SIZE 64
#define ETHASH_FULL_DATASET_ITEM_SIZE 128

#define to_le64(X) X

/** Loads 64-bit integer from given memory location as little-endian number. */
static uint64_t load_le(const uint8_t* data)
{
    /* memcpy is the best way of expressing the intention. Every compiler will
       optimize is to single load instruction if the target architecture
       supports unaligned memory access (GCC and clang even in O0).
       This is great trick because we are violating C/C++ memory alignment
       restrictions with no performance penalty. */
    uint64_t word;
    //__builtin_memcpy(&word, data, sizeof(word));
    memcpy(&word, data, sizeof(word));
    return to_le64(word);
}

static uint64_t rol(uint64_t x, unsigned s)
{
    return (x << s) | (x >> (64 - s));
}

static inline uint32_t fnv1(uint32_t u, uint32_t v) noexcept
{
    return (u * fnv_prime) ^ v;
}

static const uint64_t round_constants[24] = {
    0x0000000000000001,
    0x0000000000008082,
    0x800000000000808a,
    0x8000000080008000,
    0x000000000000808b,
    0x0000000080000001,
    0x8000000080008081,
    0x8000000000008009,
    0x000000000000008a,
    0x0000000000000088,
    0x0000000080008009,
    0x000000008000000a,
    0x000000008000808b,
    0x800000000000008b,
    0x8000000000008089,
    0x8000000000008003,
    0x8000000000008002,
    0x8000000000000080,
    0x000000000000800a,
    0x800000008000000a,
    0x8000000080008081,
    0x8000000000008080,
    0x0000000080000001,
    0x8000000080008008,
};

void ethash_keccakf1600(uint64_t state[25])
{
    /* The implementation based on the "simple" implementation by Ronny Van Keer. */

    int round;

    uint64_t Aba, Abe, Abi, Abo, Abu;
    uint64_t Aga, Age, Agi, Ago, Agu;
    uint64_t Aka, Ake, Aki, Ako, Aku;
    uint64_t Ama, Ame, Ami, Amo, Amu;
    uint64_t Asa, Ase, Asi, Aso, Asu;

    uint64_t Eba, Ebe, Ebi, Ebo, Ebu;
    uint64_t Ega, Ege, Egi, Ego, Egu;
    uint64_t Eka, Eke, Eki, Eko, Eku;
    uint64_t Ema, Eme, Emi, Emo, Emu;
    uint64_t Esa, Ese, Esi, Eso, Esu;

    uint64_t Ba, Be, Bi, Bo, Bu;

    uint64_t Da, De, Di, Do, Du;

    Aba = state[0];
    Abe = state[1];
    Abi = state[2];
    Abo = state[3];
    Abu = state[4];
    Aga = state[5];
    Age = state[6];
    Agi = state[7];
    Ago = state[8];
    Agu = state[9];
    Aka = state[10];
    Ake = state[11];
    Aki = state[12];
    Ako = state[13];
    Aku = state[14];
    Ama = state[15];
    Ame = state[16];
    Ami = state[17];
    Amo = state[18];
    Amu = state[19];
    Asa = state[20];
    Ase = state[21];
    Asi = state[22];
    Aso = state[23];
    Asu = state[24];

    for (round = 0; round < 24; round += 2)
    {
        /* Round (round + 0): Axx -> Exx */

        Ba = Aba ^ Aga ^ Aka ^ Ama ^ Asa;
        Be = Abe ^ Age ^ Ake ^ Ame ^ Ase;
        Bi = Abi ^ Agi ^ Aki ^ Ami ^ Asi;
        Bo = Abo ^ Ago ^ Ako ^ Amo ^ Aso;
        Bu = Abu ^ Agu ^ Aku ^ Amu ^ Asu;

        Da = Bu ^ rol(Be, 1);
        De = Ba ^ rol(Bi, 1);
        Di = Be ^ rol(Bo, 1);
        Do = Bi ^ rol(Bu, 1);
        Du = Bo ^ rol(Ba, 1);

        Ba = Aba ^ Da;
        Be = rol(Age ^ De, 44);
        Bi = rol(Aki ^ Di, 43);
        Bo = rol(Amo ^ Do, 21);
        Bu = rol(Asu ^ Du, 14);
        Eba = Ba ^ (~Be & Bi) ^ round_constants[round];
        Ebe = Be ^ (~Bi & Bo);
        Ebi = Bi ^ (~Bo & Bu);
        Ebo = Bo ^ (~Bu & Ba);
        Ebu = Bu ^ (~Ba & Be);

        Ba = rol(Abo ^ Do, 28);
        Be = rol(Agu ^ Du, 20);
        Bi = rol(Aka ^ Da, 3);
        Bo = rol(Ame ^ De, 45);
        Bu = rol(Asi ^ Di, 61);
        Ega = Ba ^ (~Be & Bi);
        Ege = Be ^ (~Bi & Bo);
        Egi = Bi ^ (~Bo & Bu);
        Ego = Bo ^ (~Bu & Ba);
        Egu = Bu ^ (~Ba & Be);

        Ba = rol(Abe ^ De, 1);
        Be = rol(Agi ^ Di, 6);
        Bi = rol(Ako ^ Do, 25);
        Bo = rol(Amu ^ Du, 8);
        Bu = rol(Asa ^ Da, 18);
        Eka = Ba ^ (~Be & Bi);
        Eke = Be ^ (~Bi & Bo);
        Eki = Bi ^ (~Bo & Bu);
        Eko = Bo ^ (~Bu & Ba);
        Eku = Bu ^ (~Ba & Be);

        Ba = rol(Abu ^ Du, 27);
        Be = rol(Aga ^ Da, 36);
        Bi = rol(Ake ^ De, 10);
        Bo = rol(Ami ^ Di, 15);
        Bu = rol(Aso ^ Do, 56);
        Ema = Ba ^ (~Be & Bi);
        Eme = Be ^ (~Bi & Bo);
        Emi = Bi ^ (~Bo & Bu);
        Emo = Bo ^ (~Bu & Ba);
        Emu = Bu ^ (~Ba & Be);

        Ba = rol(Abi ^ Di, 62);
        Be = rol(Ago ^ Do, 55);
        Bi = rol(Aku ^ Du, 39);
        Bo = rol(Ama ^ Da, 41);
        Bu = rol(Ase ^ De, 2);
        Esa = Ba ^ (~Be & Bi);
        Ese = Be ^ (~Bi & Bo);
        Esi = Bi ^ (~Bo & Bu);
        Eso = Bo ^ (~Bu & Ba);
        Esu = Bu ^ (~Ba & Be);


        /* Round (round + 1): Exx -> Axx */

        Ba = Eba ^ Ega ^ Eka ^ Ema ^ Esa;
        Be = Ebe ^ Ege ^ Eke ^ Eme ^ Ese;
        Bi = Ebi ^ Egi ^ Eki ^ Emi ^ Esi;
        Bo = Ebo ^ Ego ^ Eko ^ Emo ^ Eso;
        Bu = Ebu ^ Egu ^ Eku ^ Emu ^ Esu;

        Da = Bu ^ rol(Be, 1);
        De = Ba ^ rol(Bi, 1);
        Di = Be ^ rol(Bo, 1);
        Do = Bi ^ rol(Bu, 1);
        Du = Bo ^ rol(Ba, 1);

        Ba = Eba ^ Da;
        Be = rol(Ege ^ De, 44);
        Bi = rol(Eki ^ Di, 43);
        Bo = rol(Emo ^ Do, 21);
        Bu = rol(Esu ^ Du, 14);
        Aba = Ba ^ (~Be & Bi) ^ round_constants[round + 1];
        Abe = Be ^ (~Bi & Bo);
        Abi = Bi ^ (~Bo & Bu);
        Abo = Bo ^ (~Bu & Ba);
        Abu = Bu ^ (~Ba & Be);

        Ba = rol(Ebo ^ Do, 28);
        Be = rol(Egu ^ Du, 20);
        Bi = rol(Eka ^ Da, 3);
        Bo = rol(Eme ^ De, 45);
        Bu = rol(Esi ^ Di, 61);
        Aga = Ba ^ (~Be & Bi);
        Age = Be ^ (~Bi & Bo);
        Agi = Bi ^ (~Bo & Bu);
        Ago = Bo ^ (~Bu & Ba);
        Agu = Bu ^ (~Ba & Be);

        Ba = rol(Ebe ^ De, 1);
        Be = rol(Egi ^ Di, 6);
        Bi = rol(Eko ^ Do, 25);
        Bo = rol(Emu ^ Du, 8);
        Bu = rol(Esa ^ Da, 18);
        Aka = Ba ^ (~Be & Bi);
        Ake = Be ^ (~Bi & Bo);
        Aki = Bi ^ (~Bo & Bu);
        Ako = Bo ^ (~Bu & Ba);
        Aku = Bu ^ (~Ba & Be);

        Ba = rol(Ebu ^ Du, 27);
        Be = rol(Ega ^ Da, 36);
        Bi = rol(Eke ^ De, 10);
        Bo = rol(Emi ^ Di, 15);
        Bu = rol(Eso ^ Do, 56);
        Ama = Ba ^ (~Be & Bi);
        Ame = Be ^ (~Bi & Bo);
        Ami = Bi ^ (~Bo & Bu);
        Amo = Bo ^ (~Bu & Ba);
        Amu = Bu ^ (~Ba & Be);

        Ba = rol(Ebi ^ Di, 62);
        Be = rol(Ego ^ Do, 55);
        Bi = rol(Eku ^ Du, 39);
        Bo = rol(Ema ^ Da, 41);
        Bu = rol(Ese ^ De, 2);
        Asa = Ba ^ (~Be & Bi);
        Ase = Be ^ (~Bi & Bo);
        Asi = Bi ^ (~Bo & Bu);
        Aso = Bo ^ (~Bu & Ba);
        Asu = Bu ^ (~Ba & Be);
    }

    state[0] = Aba;
    state[1] = Abe;
    state[2] = Abi;
    state[3] = Abo;
    state[4] = Abu;
    state[5] = Aga;
    state[6] = Age;
    state[7] = Agi;
    state[8] = Ago;
    state[9] = Agu;
    state[10] = Aka;
    state[11] = Ake;
    state[12] = Aki;
    state[13] = Ako;
    state[14] = Aku;
    state[15] = Ama;
    state[16] = Ame;
    state[17] = Ami;
    state[18] = Amo;
    state[19] = Amu;
    state[20] = Asa;
    state[21] = Ase;
    state[22] = Asi;
    state[23] = Aso;
    state[24] = Asu;
}


union hash256
{
    uint64_t word64s[4];
    uint32_t word32s[8];
    uint8_t bytes[32];
    char str[32];
};

union hash512
{
    uint64_t word64s[8];
    uint32_t word32s[16];
    uint8_t bytes[64];
    char str[64];
};

union hash1024
{
    union hash512 hash512s[2];
    uint64_t word64s[16];
    uint32_t word32s[32];
    uint8_t bytes[128];
    char str[128];
};

union hash2048
{
    union hash512 hash512s[4];
    uint64_t word64s[32];
    uint32_t word32s[64];
    uint8_t bytes[256];
    char str[256];
};

inline hash512 fnv1(const hash512& u, const hash512& v) noexcept
{
    hash512 r;
    for (size_t i = 0; i < sizeof(r) / sizeof(r.word32s[0]); ++i)
        r.word32s[i] = fnv1(u.word32s[i], v.word32s[i]);
    return r;
}

struct epoch_context
{
    const int epoch_number;
    const int light_cache_num_items;
    const union hash512* const light_cache;
    const uint32_t* const l1_cache;
    const int full_dataset_num_items;
};

struct epoch_context_full : epoch_context
{
    hash1024* full_dataset;

    constexpr epoch_context_full(int epoch, int light_num_items,
        const hash512* light, const uint32_t* l1, int dataset_num_items,
        hash1024* dataset) noexcept
      : epoch_context{epoch, light_num_items, light, l1, dataset_num_items},
        full_dataset{dataset}
    {}
};

void keccak(
    uint64_t* out, size_t bits, const uint8_t* data, size_t size)
{
    static const size_t word_size = sizeof(uint64_t);
    const size_t hash_size = bits / 8;
    const size_t block_size = (1600 - bits * 2) / 8;

    size_t i;
    uint64_t* state_iter;
    uint64_t last_word = 0;
    uint8_t* last_word_iter = (uint8_t*)&last_word;

    uint64_t state[25] = {0};

    while (size >= block_size)
    {
        for (i = 0; i < (block_size / word_size); ++i)
        {
            state[i] ^= load_le(data);
            data += word_size;
        }

        ethash_keccakf1600(state);

        size -= block_size;
    }

    state_iter = state;

    while (size >= word_size)
    {
        *state_iter ^= load_le(data);
        ++state_iter;
        data += word_size;
        size -= word_size;
    }

    while (size > 0)
    {
        *last_word_iter = *data;
        ++last_word_iter;
        ++data;
        --size;
    }
    *last_word_iter = 0x01;
    *state_iter ^= to_le64(last_word);

    state[(block_size / word_size) - 1] ^= 0x8000000000000000;

    ethash_keccakf1600(state);

    for (i = 0; i < (hash_size / word_size); ++i)
        out[i] = to_le64(state[i]);
}

union hash256 ethash_keccak256(const uint8_t* data, size_t size)
{
    union hash256 hash;
    keccak(hash.word64s, 256, data, size);
    return hash;
}

union hash256 ethash_keccak256_32(const uint8_t data[32])
{
    union hash256 hash;
    keccak(hash.word64s, 256, data, 32);
    return hash;
}

union hash512 ethash_keccak512(const uint8_t* data, size_t size)
{
    union hash512 hash;
    keccak(hash.word64s, 512, data, size);
    return hash;
}

union hash512 ethash_keccak512_64(const uint8_t data[64])
{
    union hash512 hash;
    keccak(hash.word64s, 512, data, 64);
    return hash;
}

inline hash512 bitwise_xor(const hash512& x, const hash512& y) noexcept
{
    hash512 z;
    for (size_t i = 0; i < sizeof(z) / sizeof(z.word64s[0]); ++i)
        z.word64s[i] = x.word64s[i] ^ y.word64s[i];
    return z;
}

struct le
{
    static uint32_t uint32(uint32_t x) noexcept { return x; }
    static uint64_t uint64(uint64_t x) noexcept { return x; }

    static const hash1024& uint32s(const hash1024& h) noexcept { return h; }
    static const hash512& uint32s(const hash512& h) noexcept { return h; }
    static const hash256& uint32s(const hash256& h) noexcept { return h; }
};

struct item_state
{
    const hash512* const cache;
    const int64_t num_cache_items;
    const uint32_t seed;

    hash512 mix;

    item_state(const epoch_context& context, int64_t index) noexcept
      : cache{context.light_cache},
        num_cache_items{context.light_cache_num_items},
        seed{static_cast<uint32_t>(index)}
    {
        mix = cache[index % num_cache_items];
        mix.word32s[0] ^= le::uint32(seed);
        mix = le::uint32s(ethash_keccak512_64(mix.bytes));
    }

    void update(uint32_t round) noexcept
    {
        static constexpr size_t num_words = sizeof(mix) / sizeof(uint32_t);
        const uint32_t t = fnv1(seed ^ round, mix.word32s[round % num_words]);
        const int64_t parent_index = t % num_cache_items;
        mix = fnv1(mix, le::uint32s(cache[parent_index]));
    }

    hash512 final() noexcept { return ethash_keccak512_64(le::uint32s(mix).bytes); }
};

static int is_odd_prime(int number)
{
    int d;

    /* Check factors up to sqrt(number).
       To avoid computing sqrt, compare d*d <= number with 64-bit precision. */
    for (d = 3; (int64_t)d * (int64_t)d <= (int64_t)number; d += 2)
    {
        if (number % d == 0)
            return 0;
    }

    return 1;
}

int find_largest_prime(int upper_bound)
{
    int n = upper_bound;

    if (n < 2)
        return 0;

    if (n == 2)
        return 2;

    /* If even number, skip it. */
    if (n % 2 == 0)
        --n;

    /* Test descending odd numbers. */
    while (!is_odd_prime(n))
        n -= 2;

    return n;
}

int find_epoch_number(const hash256& seed) noexcept
{
    static constexpr int num_tries = 30000;  // Divisible by 16.

    // Thread-local cache of the last search.
    static thread_local int cached_epoch_number = 0;
    static thread_local hash256 cached_seed = {};

    // Load from memory once (memory will be clobbered by keccak256()).
    const uint32_t seed_part = seed.word32s[0];
    const int e = cached_epoch_number;
    hash256 s = cached_seed;

    if (s.word32s[0] == seed_part)
        return e;

    // Try the next seed, will match for sequential epoch access.
    s = ethash_keccak256(s.bytes, 32);
    if (s.word32s[0] == seed_part)
    {
        cached_seed = s;
        cached_epoch_number = e + 1;
        return e + 1;
    }

    // Search for matching seed starting from epoch 0.
    s = {};
    for (int i = 0; i < num_tries; ++i)
    {
        if (s.word32s[0] == seed_part)
        {
            cached_seed = s;
            cached_epoch_number = i;
            return i;
        }

        s = ethash_keccak256(s.bytes, 32);
    }

    return -1;
}

hash256 calculate_epoch_seed(int epoch_number) noexcept
{
    hash256 epoch_seed = {};
    for (int i = 0; i < epoch_number; ++i)
        epoch_seed = ethash_keccak256_32(epoch_seed.bytes);
    return epoch_seed;
}

int calculate_light_cache_num_items(int epoch_number) noexcept
{
    static constexpr int item_size = sizeof(hash512);
    static constexpr int num_items_init = light_cache_init_size / item_size;
    static constexpr int num_items_growth = light_cache_growth / item_size;
    static_assert(
        light_cache_init_size % item_size == 0, "light_cache_init_size not multiple of item size");
    static_assert(
        light_cache_growth % item_size == 0, "light_cache_growth not multiple of item size");

    int num_items_upper_bound = num_items_init + epoch_number * num_items_growth;
    int num_items = find_largest_prime(num_items_upper_bound);
    return num_items;
}

int calculate_full_dataset_num_items(int epoch_number) noexcept
{
    static constexpr int item_size = sizeof(hash1024);
    static constexpr int num_items_init = full_dataset_init_size / item_size;
    static constexpr int num_items_growth = full_dataset_growth / item_size;
    static_assert(full_dataset_init_size % item_size == 0,
        "full_dataset_init_size not multiple of item size");
    static_assert(
        full_dataset_growth % item_size == 0, "full_dataset_growth not multiple of item size");

    int num_items_upper_bound = num_items_init + epoch_number * num_items_growth;
    int num_items = find_largest_prime(num_items_upper_bound);
    return num_items;
}

hash2048 calculate_dataset_item_2048(const epoch_context& context, uint32_t index) noexcept
{
    item_state item0{context, int64_t(index) * 4};
    item_state item1{context, int64_t(index) * 4 + 1};
    item_state item2{context, int64_t(index) * 4 + 2};
    item_state item3{context, int64_t(index) * 4 + 3};

    for (uint32_t j = 0; j < full_dataset_item_parents; ++j)
    {
        item0.update(j);
        item1.update(j);
        item2.update(j);
        item3.update(j);
    }

    return hash2048{{item0.final(), item1.final(), item2.final(), item3.final()}};
}

inline constexpr size_t get_light_cache_size(int num_items) noexcept
{
    return static_cast<size_t>(num_items) * ETHASH_LIGHT_CACHE_ITEM_SIZE; //light_cache_item_size;
}

inline constexpr uint64_t get_full_dataset_size(int num_items) noexcept
{
    return static_cast<uint64_t>(num_items) * ETHASH_FULL_DATASET_ITEM_SIZE;
}

void build_light_cache(
    hash512 cache[], int num_items, const hash256& seed) noexcept
{
    hash512 item = ethash_keccak512(seed.bytes, sizeof(seed));
    cache[0] = item;
    for (int i = 1; i < num_items; ++i)
    {
        item = ethash_keccak512(item.bytes, sizeof(item));
        cache[i] = item;
    }

    for (int q = 0; q < light_cache_rounds; ++q)
    {
        for (int i = 0; i < num_items; ++i)
        {
            const uint32_t index_limit = static_cast<uint32_t>(num_items);

            // Fist index: 4 first bytes of the item as little-endian integer.
            const uint32_t t = le::uint32(cache[i].word32s[0]);
            const uint32_t v = t % index_limit;

            // Second index.
            const uint32_t w = static_cast<uint32_t>(num_items + (i - 1)) % index_limit;

            const hash512 x = bitwise_xor(cache[v], cache[w]);
            cache[i] = ethash_keccak512(x.bytes, sizeof(x));
        }
    }
}

epoch_context_full* create_epoch_context(
    int epoch_number, bool full) noexcept
{
    static_assert(sizeof(epoch_context_full) < sizeof(hash512), "epoch_context too big");
    static constexpr size_t context_alloc_size = sizeof(hash512);

    // TODO - iquidus
    int epoch_ecip1099 = epoch_number;
    if (epoch_number >= ecip_1099_activation_epoch)
    {
        // note, int truncates, it doesnt round, 10 == 10.5. So this is ok.
        epoch_ecip1099 = epoch_number/2;
    }

    const int light_cache_num_items = calculate_light_cache_num_items(epoch_ecip1099);
    const int full_dataset_num_items = calculate_full_dataset_num_items(epoch_ecip1099);
    const size_t light_cache_size = get_light_cache_size(light_cache_num_items);
    const size_t full_dataset_size =
        full ? static_cast<size_t>(full_dataset_num_items) * sizeof(hash1024) :
               l1_cache_size;

    const size_t alloc_size = context_alloc_size + light_cache_size + full_dataset_size;

    char* const alloc_data = static_cast<char*>(std::calloc(1, alloc_size));
    if (!alloc_data)
        return nullptr;  // Signal out-of-memory by returning null pointer.

    hash512* const light_cache = reinterpret_cast<hash512*>(alloc_data + context_alloc_size);
    const hash256 epoch_seed = calculate_epoch_seed(epoch_number);
    build_light_cache(light_cache, light_cache_num_items, epoch_seed);

    uint32_t* const l1_cache =
        reinterpret_cast<uint32_t*>(alloc_data + context_alloc_size + light_cache_size);

    hash1024* full_dataset = full ? reinterpret_cast<hash1024*>(l1_cache) : nullptr;

    epoch_context_full* const context = new (alloc_data) epoch_context_full{
        epoch_number,
        light_cache_num_items,
        light_cache,
        l1_cache,
        full_dataset_num_items,
        full_dataset,
    };

    auto* full_dataset_2048 = reinterpret_cast<hash2048*>(l1_cache);
    for (uint32_t i = 0; i < l1_cache_size / sizeof(full_dataset_2048[0]); ++i)
        full_dataset_2048[i] = calculate_dataset_item_2048(*context, i);
    return context;
}

template <class T>
std::string toHex(T const& _data, int _w = 2)
{
    std::ostringstream ret;
    unsigned ii = 0;
    for (auto i : _data)
        ret << std::hex << std::setfill('0') << std::setw(ii++ ? 2 : _w)
            << (int)(typename std::make_unsigned<decltype(i)>::type)i;
    return ret.str();
}

NAN_METHOD(echo) {
    std::ostringstream oss;
    oss << "a + " << "b";
    info.GetReturnValue().Set(Nan::New(oss.str()).ToLocalChecked());
}

struct epoch_context_full* ctx;

NAN_METHOD(getEpochContext) {
    std::ostringstream oss;
    double d = info[0]->IsNumber() ? Nan::To<double>(info[0]).FromJust() : 0;
    
    // std::cout << "Epoch: " << d;

    // call get epoch context
    struct epoch_context_full* ctx = create_epoch_context(d, false);

    oss << "{";
    oss << "\"epochNumber\":" << ctx->epoch_number << ",";
    oss << "\"lightNumItems\":" << ctx->light_cache_num_items << ",";
    oss << "\"lightSize\":" << get_light_cache_size(ctx->light_cache_num_items) << ",";
    oss << "\"dagNumItems\":" << ctx->full_dataset_num_items << ",";
    oss << "\"dagSize\":" << get_full_dataset_size(ctx->full_dataset_num_items) << ",";
    //  render as buffer items/hash512s
    oss << "\"lightCache\":[";
    for (unsigned i = 0; i < ctx->light_cache_num_items; ++i) {
        oss << "\"0x" << toHex(ctx->light_cache[i].bytes) << "\",";
    }
    long currpos = oss.tellp();
    oss.seekp(currpos - 1);
    oss << "]}";

    info.GetReturnValue().Set(Nan::New(oss.str()).ToLocalChecked());
}

NAN_METHOD(getEpochContextBin) {
    std::ostringstream oss;
    double d = info[0]->IsNumber() ? Nan::To<double>(info[0]).FromJust() : 0;
    
    // std::cout << "Epoch: " << d;

    // call get epoch context
    ctx = create_epoch_context(d, false);
    ctx->full_dataset = nullptr;
    // ctx->epoch_number = 455;
    uint8_t buf[sizeof(epoch_context_full)];
    
    // FIXME: and bit align in CL
    // check by recopying back to struct
    memcpy(&buf, (void*)ctx, sizeof(epoch_context_full));

    oss << "{\"bin\":[";
    oss << "\"0x" << toHex(buf) << "\" ],";
    oss << "\"lightNumItems\":" << ctx->light_cache_num_items << ",";
    oss << "\"lightSize\":" << get_light_cache_size(ctx->light_cache_num_items) << ",";
    oss << "\"dagSize\":" << get_full_dataset_size(ctx->full_dataset_num_items) << ",";
    oss << "\"dagNumItems\":" << ctx->full_dataset_num_items << ",";
    // oss << "\"lightCache\":[";
    // for (unsigned i = 0; i < ctx->light_cache_num_items; ++i) {
    //     oss << "\"0x" << toHex(ctx->light_cache[i].bytes) << "\",";
    // }
    long currpos = oss.tellp();
    oss.seekp(currpos - 1);
    // oss << "]}";
    oss << "}";

    info.GetReturnValue().Set(Nan::New(oss.str()).ToLocalChecked());
}

NAN_METHOD(getLightCache) {
    info.GetReturnValue().Set(Nan::CopyBuffer((char*)ctx->light_cache, 
        ctx->light_cache_num_items * 64).ToLocalChecked());
}

using v8::FunctionTemplate;

NAN_MODULE_INIT(InitAll) {
    Nan::Set(target, Nan::New("echo").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(echo)).ToLocalChecked());

    Nan::Set(target, Nan::New("getEpochContext").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(getEpochContext)).ToLocalChecked());

    Nan::Set(target, Nan::New("getEpochContextBin").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(getEpochContextBin)).ToLocalChecked());

    Nan::Set(target, Nan::New("getLightCache").ToLocalChecked(),
    Nan::GetFunction(Nan::New<FunctionTemplate>(getLightCache)).ToLocalChecked());

}

NODE_MODULE(libeth, InitAll)