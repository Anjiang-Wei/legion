#include "myalgorithm.h"

void test()
{
    int number = 2;
    std::vector<int> launch_domain = {1, 1, 1};
    auto res = precise_enumerate(number, launch_domain);
    auto res2 = greedy(number, launch_domain);
    printvec(res);
    printvec(res2);
}

void summary_test()
{
        std::vector<std::vector<int>> results;
    int improve_cnt = 0;
    float improve_perc_total = 0.0;
    float best_improve_perc = 0.0;
    int best_node_cnt, best_dx, best_dy;
    best_node_cnt = best_dx = best_dy = 0;
    int node_num_max, x_max, y_max;
    node_num_max = 128;
    x_max = y_max = 128;
    for (int node_num = 2; node_num < node_num_max; node_num++)
    {
        for (int domain_x = 2; domain_x < x_max; domain_x++)
        {
            for (int domain_y = 2; domain_y < y_max; domain_y++)
            {
                if (domain_x * domain_y < node_num)
                    continue;
                std::vector<int> launch_domain = std::vector<int>{domain_x, domain_y};
                results.push_back(greedy(node_num, launch_domain)); // Default Mapper's heursitics
                // results.push_back(brute_force(node_num, launch_domain, true)); // minimize maximal difference
                // results.push_back(brute_force(node_num, launch_domain, false)); // minimize real cost
                results.push_back(precise_enumerate(node_num, launch_domain)); // smarter algorithm to minimize cost
                float cur_improve_perc = judge(results, launch_domain, node_num, domain_x, domain_y);
                if (cur_improve_perc > 0)
                {
                    improve_cnt++;
                    improve_perc_total += cur_improve_perc;
                    if (cur_improve_perc > best_improve_perc)
                    {
                        best_improve_perc = cur_improve_perc;
                        best_node_cnt = node_num;
                        best_dx = domain_x;
                        best_dy = domain_y;
                    }
                }
                results.clear();
            }
        }
    }
    int total_cnt = (node_num_max - 2) * (x_max - 2) * (y_max - 2);
    printf("improve percentage= %d / %d = %lf, average improve perc = %lf,\
        best improve perc = %lf, coming from %d and (%d, %d)\n", 
        improve_cnt, total_cnt, improve_cnt * 1.0 / total_cnt, improve_perc_total / improve_cnt,
        best_improve_perc, best_node_cnt, best_dx, best_dy);
}

int main()
{
    test();
    return 0;
}
