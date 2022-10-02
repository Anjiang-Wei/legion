#include <algorithm>
#include <vector>
#include <set>
#include <assert.h>
#include <string>
#include <iostream>
#include <sstream>
#include <iterator>
#include <map>
#include <unordered_map>
#include <functional>
#include <numeric>
#include <math.h>

void printvec(const std::vector<int>& my_vector);

// Helper function: factorize a number (big_number) into sorted prime factors (factors_result)
void generate_prime_factor(int big_number, std::vector<int>& factors_result)
{
    auto generate_factors = [&](int factor)
    {
        while (big_number % factor == 0)
        {
            factors_result.push_back(factor);
            big_number /= factor;
        }
    };
    // Fundamental theorem of arithmetic time!
    const unsigned num_primes = 32;
    const int primes[num_primes] = { 2, 3, 5, 7, 11, 13, 17, 19, 
                                    23, 29, 31, 37, 41, 43, 47, 53,
                                    59, 61, 67, 71, 73, 79, 83, 89,
                                    97, 101, 103, 107, 109, 113, 127, 131 };
    // Increase the size of the prime number table if you ever hit this
    assert(big_number <= (primes[num_primes-1] * primes[num_primes-1]));
    for (int i = 0; i < num_primes; i++)
    {
        if (primes[i] * primes[i] > big_number) // There is at most 1 prime whose index >= i
            break;
        generate_factors(primes[i]);
        if (big_number == 1)
            break;
    }
    if (big_number > 1) // E.g., we need to put 137 into the result for 2 * 137
        factors_result.push_back(big_number);
}

std::unordered_map<int, std::map<std::vector<int>, std::vector<int>>> cache_greedy;

// DefaultMapper's algorithm
std::vector<int> greedy(const int number, const std::vector<int>& launch_domain)
{
    if (cache_greedy.count(number) > 0)
    {
        auto value = cache_greedy.at(number);
        if (value.count(launch_domain) > 0)
        {
            return value.at(launch_domain);
        }
    }
    int dim = launch_domain.size();
    std::vector<int> result;
    result.resize(dim, 1);
    if (number == 1)
    {
        return result;
    }
    // factorize number into prime_nums (sorted from smallest to largest)
    std::vector<int> prime_nums;
    generate_prime_factor(number, prime_nums);
    // Assign prime nums onto the dimensions
    // from the largest primes down to the smallest, in a greedy approach
    std::vector<double> domain_vec;
    for (int i = 0; i < dim; i++)
        domain_vec.push_back((double) launch_domain[i]); // integer to double

    // from the largest primes down to the smallest
    for (int idx = prime_nums.size() - 1; idx >= 0; idx--)
    {
        // Find the dimension with the biggest domain_vec
        int next_dim = std::max_element(domain_vec.begin(), domain_vec.end()) - domain_vec.begin();
        int next_prime = prime_nums[idx];
        result[next_dim] *= next_prime;
        domain_vec[next_dim] /= next_prime;
    }
    if (cache_greedy.count(number) == 0)
    {
        std::map<std::vector<int>, std::vector<int>> empty;
        cache_greedy.insert({number, empty});
    }
    cache_greedy.at(number).insert({launch_domain, result});
    return result;
}

// Helper function: divide a / b elementwise, return float
std::vector<float> divide(const std::vector<int>& a, const std::vector<int>& b)
{
    assert(a.size() == b.size());
    std::vector<float> result;
    for (int i = 0; i < a.size(); i++)
    {
        result.push_back((float) a[i] * 1.0 / (float) b[i]);
    }
    return result;
}


// This is not the best enumeration algorithm. Dynamic programming here is equivalent to brute force + memorization
// proxy: True: minimize maximal difference; False: directly minimizing sum of O_i/L_i 
std::vector<int> brute_force(const int number, const std::vector<int>& launch_domain, const bool proxy)
{
    // number can be regarded as #nodes
    int dim = launch_domain.size();
    std::vector<int> result;
    result.resize(dim, 1);
    if (number == 1)
    {
        return result;
    }

    // factorize workload constant into prime_nums (sorted from smallest to largest)
    std::vector<int> prime_nums;
    generate_prime_factor(number, prime_nums);
    // number = p_1 * p_2 * ... * p_m
    // number = o_1 * o_2 * ... * o_{dim}
    // There are dim^{m} different ways to map p_i (i<=m) to o_j (j<=dim)

    // Use <long long int> to represent the mapping state
    // Each digit is a base-{dim} number, and it has at most m digits
    // The first p_i has weight 1={base}^0, the next p_i has weight {base}^1, the next has {base}^2...
    
    // state_o_vec[k][state]: after placing first k primes (mapping recorded in state), the vector of o_i
    // vector<int>: o_1, o_2, ..., o_n
    std::vector<std::unordered_map<long long int, std::vector<int>>> state_o_vec;

    // Before choose any mapping, all o_{i} should be initialized to 1
    std::vector<int> vec_init(result);
    std::unordered_map<long long int, std::vector<int>> state_o_vec_0({{0, vec_init}});
    state_o_vec.push_back(state_o_vec_0);

    // For step k (selecting the p_k's mapping):
    // Enumerate all possible state \in { state | state_of_first_{k-1}_primes' mapping choices } 
    // Enumerate all possible mapping choices j for p_k (j can choose any index from 0 to dim)
    // state_o_vec[k][state \union p_k] =  state_w[k-1][state].update(j, multiplied by p_k)

    for (int i = 0; i < prime_nums.size(); i++) // Each step decides the mapping for p_i
    {
        std::unordered_map<long long int, std::vector<int>> state_o_vec_i;
        for (const auto item: state_o_vec[i])
        {
            const long long int pre_state = item.first;
            std::vector<int> pre_o_vec = item.second;
            for (int j = 0; j < dim; j++) // for each prime i, enumerate every o_j to be mapped
            {
                long long int new_state = pre_state + j * pow(dim, i);
                std::vector<int> new_o_vec(pre_o_vec);
                new_o_vec[j] *= prime_nums[i];
                assert(state_o_vec_i.count(new_state) == 0); // must be unique
                state_o_vec_i.insert({new_state, new_o_vec});
            }
        }
        state_o_vec.push_back(state_o_vec_i);
    }

    // minimize maximum difference: iterate each state in state_max[m][*]
    // Compute the workload vector w_i: w_i = L_i / o_i
    // Proxy==True: find the smallest (max_element(w_i) - min_element(w_i))
    // Proxy==False: minimize sum{o_i / L_i} (proxy = False)
    std::unordered_map<long long int, std::vector<int>> final_states = state_o_vec[state_o_vec.size()-1];
    float minimal = INT32_MAX;
    for (const auto& item : final_states)
    {
        std::vector<int> o_vec = item.second;
        float cur = 0;
        if (proxy)
        {
            // std::vector<float> w_vec = std::transform(launch_domain.begin(), launch_domain.end(), 
            //     o_vec.begin(), o_vec.end(), std::divides<int>());
            std::vector<float> w_vec = divide(launch_domain, o_vec);
            cur = (*std::max_element(w_vec.begin(), w_vec.end())) - (*std::min_element(w_vec.begin(), w_vec.end()));
        }
        else
        {
            std::vector<float> o_over_L = divide(o_vec, launch_domain);
            cur = std::accumulate(o_over_L.begin(), o_over_L.end(), 0.0);
        }
        if (cur < minimal)
        {
            minimal = cur;
            result = o_vec;
        }
    }
    return result;
    // Complexity analysis: suppose C=number, dim=launch_domain.size()
    // #State: #choices for mapping C's prime numbers into dim positions
    // #C's prime numbers: O(log(C, base=2))
    // #State: O({dim}^{log(C)})
    // Iterative picking prime: O(dim * {dim}^{log(C)}) = O({dim}^{log(C)+1})
    // Last For-loop: picking maximum/minimum among {dim} numbers for each state: O({dim}^{log(C)+1})
    // Complexity: O({dim}^{log(C)})
}


// quick pow for int
inline int binpow(int a, int b)
{
    int res = 1;
    while (b > 0)
    {
        if (b & 1)
        {
            res = res * a;
        }
        a = a * a;
        b >>= 1;
    }
    return res;
}

void generate_prime_factorization(const int number, std::unordered_map<int, int>& result, std::vector<int>& unique_prime)
{
    std::vector<int> prime_nums;
    generate_prime_factor(number, prime_nums);

    std::set<int> prime_num_set(prime_nums.begin(), prime_nums.end());
    unique_prime = std::vector<int>(prime_num_set.begin(), prime_num_set.end());

    std::multiset<int> prime_num_multiset(prime_nums.begin(), prime_nums.end());
    int total_elements = 0;
    for (int i = 0; i < prime_nums.size(); i++)
    {
        if (result.count(prime_nums[i]) == 0)
        {
            int appear_times = prime_num_multiset.count(prime_nums[i]);
            total_elements += appear_times;
            result.insert({prime_nums[i], appear_times});
        }
    }
    assert(total_elements == prime_nums.size());
}

// result contain all possible ways of placement to decompose {prime}^{power} into {num_places}
void enumerate_placement(const int prime, int power, int num_places, 
                         std::vector<int> partial_result, std::vector<std::vector<int>>& final_result)
{
    if (power < 0)
    {
        assert(false);
    }
    if (num_places == 1)
    {
        int last_element = binpow(prime, power);
        partial_result.push_back(last_element);
        final_result.push_back(partial_result);
        return;
    }
    int cur_element = 1;
    for (int i = 0; i <= power; i++)
    {
        partial_result.push_back(cur_element);
        enumerate_placement(prime, power - i, num_places - 1, partial_result, final_result);
        partial_result.pop_back();
        cur_element *= prime; // cur_element = {prime}^{i}
    }
}

void cartesian_product(std::vector<int> unique_prime, 
                       const std::unordered_map<int, std::vector<std::vector<int>>>& prime_placement,
                       const std::vector<int>& partial_result,
                       std::vector<std::vector<int>>& final_result)
{
    if (unique_prime.size() == 0)
    {
        final_result.push_back(partial_result);
        return;
    }
    int prime = unique_prime[unique_prime.size()-1];
    std::vector<std::vector<int>> all_placement = prime_placement.at(prime);
    unique_prime.pop_back();

    std::vector<int> new_partial_result(partial_result);
    for (auto item : all_placement)
    {
        std::transform(partial_result.begin(), partial_result.end(), item.begin(), new_partial_result.begin(), std::multiplies<int>());
        cartesian_product(unique_prime, prime_placement, new_partial_result, final_result);
    }
}

// the number of ways to choose k elements from n elements
int C(int n, int k)
{
    if (k == 0 || k == n)
        return 1;
    int ans = 1;
    for (int i = 1; i <= k; i++)
    {
        ans = ans * (n - i + 1) / i; // Never shortened as *= because of integer division problem
    }
    return ans;
}

std::unordered_map<int, std::map<std::vector<int>, std::vector<int>>> cache_precise_enumerate;

std::vector<int> precise_enumerate(int number, const std::vector<int>& launch_domain)
{
    if (cache_precise_enumerate.count(number) > 0)
    {
        auto value = cache_precise_enumerate.at(number);
        if (value.count(launch_domain) > 0)
        {
            return value.at(launch_domain);
        }
    }
    // number can be regarded as #nodes
    int dim = launch_domain.size();
    std::vector<int> result;
    result.resize(dim, 1);
    if (number == 1)
    {
        return result;
    }
    // number = p1^a1 * p2^a2 * p3^a3 * ...
    // prime_factor[p_i] = a_i
    // unique_prime: p1, p2, ...
    std::unordered_map<int, int> prime2power;
    std::vector<int> unique_prime;
    generate_prime_factorization(number, prime2power, unique_prime);

    // prime_placement[p_i] records different ways to decompose {p_i}^{a_i} into {dim} places (each way is a {dim}-sized vector), 
    std::unordered_map<int, std::vector<std::vector<int>>> prime_placement;
    int total_choices = 1;
    for (int i = 0; i < unique_prime.size(); i++)
    {
        int prime_num = unique_prime[i];
        int power = prime2power.at(prime_num);
        std::vector<std::vector<int>> ways;
        enumerate_placement(prime_num, power, dim, std::vector<int>(), ways);
        int num_ways = C(power + dim - 1, dim - 1);
        assert(ways.size() == num_ways);
        total_choices *= num_ways;
        prime_placement.insert({prime_num, ways});
    }
    // all possible ways to decompose {number} into {dim} places
    std::vector<std::vector<int>> choices;
    cartesian_product(unique_prime, prime_placement, std::vector<int>(result), choices);
    assert(choices.size() == total_choices);

    float minimal = INT32_MAX;
    for (const auto& o_vec : choices)
    {
        std::vector<float> o_over_L = divide(o_vec, launch_domain);
        float cur = std::accumulate(o_over_L.begin(), o_over_L.end(), 0.0);
        if (cur < minimal)
        {
            minimal = cur;
            result = o_vec;
        }
    }
    if (cache_precise_enumerate.count(number) == 0)
    {
        std::map<std::vector<int>, std::vector<int>> empty;
        cache_precise_enumerate.insert({number, empty});
    }
    cache_precise_enumerate.at(number).insert({launch_domain, result});
    return result;
}

inline std::string myvec2str(const std::vector<int>& my_vector)
{
    std::stringstream result;
    std::copy(my_vector.begin(), my_vector.end(), std::ostream_iterator<int>(result, " "));
    return result.str();
}

inline std::string myvec2str(const std::vector<float>& my_vector)
{
    std::stringstream result;
    std::copy(my_vector.begin(), my_vector.end(), std::ostream_iterator<float>(result, " "));
    return result.str();
}

void printvec(const std::vector<int>& my_vector)
{
    std::cout << myvec2str(my_vector) << std::endl;
}

void printvec(const std::vector<float>& my_vector)
{
    std::cout << myvec2str(my_vector) << std::endl;
}

float judge(std::vector<std::vector<int>> candidates, std::vector<int> launch_space,
            int node_num=0, int dx=0, int dy=0)
{
    float best_num = INT32_MAX;
    int best_idx = 0;
    std::vector<float> results;
    for (int i = 0; i < candidates.size(); i++)
    {
        // printf("Result:\n");
        // printvec(candidates[i]);
        std::vector<float> o_over_L = divide(candidates[i], launch_space);
        float cur = std::accumulate(o_over_L.begin(), o_over_L.end(), 0.0); // Never use 0 to replace 0.0
        results.push_back(cur);
        if (cur <= best_num)
        {
            best_num = cur;
            best_idx = i;
        }
    }
    // assert(fabs(results[results.size()-1] - results[results.size()-2]) < 0.00001);
    float perc_improve = 0.0;
    for (int i = 0; i < results.size(); i++)
    {
        if (fabs(results[i] - best_num) > 0.000001)
        {
            printf("Find nonequal results for node_num = %d, launch_domain = (%d, %d)\n",
                node_num, dx, dy);
            printvec(results);
            printf("%d is worse, %lf - %lf = diff = %lf\n", i, results[i], best_num, results[i]-best_num);
            printf("Optimal's orientation is from %d: ", best_idx);
            printvec(candidates[best_idx]);
            printf("Suboptimal's orientation is from %d:", i);
            printvec(candidates[i]);
        }
    }
    if (results[best_idx] < results[0])
    {
        float delta = results[0] - results[best_idx];
        perc_improve = delta / results[0];
    }
    // assert(best_idx == 3);
    assert(best_idx == 1);
    return perc_improve;
}
