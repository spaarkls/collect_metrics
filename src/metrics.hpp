#pragma once

#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include <fstream>
#include <sstream>
#include <memory>
#include <vector>

#undef DATETIME_STR_SIZE
#define DATETIME_STR_SIZE 64

namespace Metrics {

inline constexpr double PERCENT_RATIO = 100.0;

class Metric {
protected:
    std::string title_metric;
    std::string metric_value;
    std::mutex mtx;
    std::chrono::steady_clock::time_point last_update;
    std::chrono::steady_clock::duration update_interval;
    
public:
    Metric(const std::string& name_metric, std::chrono::steady_clock::duration interval = std::chrono::seconds(1));
    virtual ~Metric();
    
    virtual void update(void) = 0;
    
    virtual void try_update(void);

    virtual std::string collect(void);
};


class DateMetric : public Metric {

public:
    DateMetric(const std::string& name_metric, std::chrono::steady_clock::duration interval = std::chrono::seconds(1));

    virtual void update(void) override final;

};


class CPUMetric : public Metric {
    struct Load {
        unsigned long idl {};
        unsigned long busy {};
        unsigned long total {};
    };
    
    Load read_cpu(void);
    Load prev_load;    
    int kernels;

public:
    CPUMetric(const std::string& name_metric, std::chrono::steady_clock::duration interval = std::chrono::seconds(1)); 

    virtual void update(void) override final;
};


class MetricManager {
    std::vector<std::shared_ptr<Metric>> metrics;
    std::thread th;
    std::thread write_th;
    std::string filename;
    bool running = true;

    void start_loop(void);
    void write_loop(const std::string& filename, std::chrono::milliseconds interval);

public:
    MetricManager(std::vector<std::shared_ptr<Metric>>&& metrics);
    MetricManager() = default;
    
    void add_metric(std::shared_ptr<Metric> metric);

    void run(const std::string& filename, std::chrono::milliseconds interval = std::chrono::milliseconds(1000));

    void stop(void);

    std::string collect(void);

};


}