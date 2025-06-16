#include <iostream>
#include "metrics.hpp"


int main(void) {
    std::cout << "Hello world\n";
    std::vector<std::shared_ptr<Metrics::Metric>> arr {std::make_shared<Metrics::DateMetric>("Date: "),
                                                       std::make_shared<Metrics::CPUMetric>("CPU: "),
                                                       std::make_shared<Metrics::HttpRequestMetric>("PRS: ")};
    
    
    
    Metrics::MetricManager m(std::move(arr));
    m.run("test.txt");

    std::thread sim_http_request ([] () {
        while (true) {
            for (int i = 0; i < 20; i++) {
                Metrics::count_http_request();
                if (i % 2 == 0) {
                    Metrics::count_http_request();
                    std::this_thread::sleep_for(std::chrono::seconds(2));
                }
                if (i % 2 != 0) {
                    int pos = 0;
                    while (pos != 20) {
                        Metrics::count_http_request();
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        pos++;
                    }                
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1500));
            }
        }
    });
    sim_http_request.detach();

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        

        std::cout << m.collect() << std::endl;
    }

    return 0;
}