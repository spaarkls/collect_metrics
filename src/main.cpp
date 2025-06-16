#include <iostream>
#include "metrics.hpp"


int main(void) {
    std::cout << "Hello world\n";
    std::vector<std::shared_ptr<Metrics::Metric>> arr {std::make_shared<Metrics::DateMetric>("Date: "),
                                                       std::make_shared<Metrics::CPUMetric>("CPU: ")};
    
    
    
    Metrics::MetricManager m(std::move(arr));
    m.run("test.txt");
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << m.collect() << std::endl;
    }

    return 0;
}