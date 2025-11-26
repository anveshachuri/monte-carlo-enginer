#include "mc_engine/core/types.hpp"
#include "mc_engine/models/gbm.hpp"
#include "mc_engine/payoffs/european.hpp"
#include "mc_engine/payoffs/asian.hpp"
#include "mc_engine/pricing/monte_carlo.hpp"
#include <fstream>
#include <iostream>
#include <vector>

using namespace mc_engine;

int main() {
    std::vector<Size> N = {1000, 5000, 10000, 20000, 50000, 100000};

    MarketParams params(100.0, 100.0, 0.05, 0.2, 1.0);

    std::ofstream file("asian_european_comparison.csv");
    file << "option_type,num_paths,price,std_error\n";

    for (Size n : N) {
        SimulationConfig config(n, 252, 42);

        // European
        EuropeanPayoff call(100.0, OptionType::Call);
        MonteCarloEngine mc_eur(params, config, call);
        auto eur = mc_eur.price();

        file << "European," << n << ","
             << eur.price << "," << eur.std_error << "\n";

        // Asian
        AsianArithmeticPayoff asian(100.0, OptionType::Call);
        MonteCarloEngine mc_asian(params, config, asian);
        auto as = mc_asian.price();

        file << "Asian," << n << ","
             << as.price << "," << as.std_error << "\n";
    }

    file.close();
    std::cout << "Generated asian_european_comparison.csv\n";
}
