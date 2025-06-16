#include "metrics.hpp"


// Basic class Metric
Metrics::Metric::Metric(const std::string& name_metric, std::chrono::steady_clock::duration interval) 
    : title_metric(name_metric), update_interval(interval), last_update(std::chrono::steady_clock::now() - interval) 
    { }
    
Metrics::Metric::~Metric() = default;

void Metrics::Metric::try_update(void) {
    std::chrono::steady_clock::time_point cur_time = std::chrono::steady_clock::now();
    if (cur_time - last_update >= update_interval) {
        update();
        last_update = cur_time;
    }
}

std::string Metrics::Metric::collect(void) {
    std::lock_guard<std::mutex> lock(mtx);
    if (metric_value.empty()) {
        return "";
    }
    return title_metric + " " + metric_value;
}


// class DateMetric;
Metrics::DateMetric::DateMetric(const std::string& name_metric, std::chrono::steady_clock::duration interval) 
    : Metric(name_metric, interval)
    { }

void Metrics::DateMetric::update(void) {
    time_t cur_time = time(NULL);
    tm* t = localtime(&cur_time);
    char buffer[DATETIME_STR_SIZE];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", t);
    metric_value = std::string(buffer);
}


// class CPUMetric
Metrics::CPUMetric::CPUMetric(const std::string& name_metric, std::chrono::steady_clock::duration interval) 
    : Metric(name_metric, interval) {
    
    prev_load = read_cpu();
}

Metrics::CPUMetric::Load Metrics::CPUMetric::read_cpu(void) {
    
    std::ifstream file("/proc/stat");
    std::string line;
    Load load {};
    kernels = 0;
    if (!file.is_open()) { return load; }
    
    while (std::getline(file, line) && line.rfind("cpu", 0) == 0) {
        if (kernels == 0) { kernels++; continue; }
        kernels++;
        std::istringstream iss(line);
        std::string cpu;
        unsigned long user{}, nice{}, system{}, idle{}, iowait{}, irq{}, softirq{}, steal{};
        iss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;
        load.idl += (iowait + idle);
        load.busy += (user + nice + system + irq + softirq + steal);
    }
    kernels--;
    load.total = (load.idl + load.busy);
    return load;
}

void Metrics::CPUMetric::update(void) {
    Load cur_load = read_cpu();

    unsigned long d_busy = cur_load.busy - prev_load.busy;
    unsigned long d_total = cur_load.total - prev_load.total;

    double p_cpu = PERCENT_RATIO * d_busy / d_total;
    double load_cores = p_cpu * kernels / PERCENT_RATIO;
    
    prev_load = cur_load;
    {
        std::lock_guard<std::mutex> lock(mtx);
        metric_value = std::to_string(load_cores) + "/" + std::to_string(kernels);
    }

}


Metrics::HttpRequestMetric::HttpRequestMetric(const std::string& name_metric, std::chrono::steady_clock::duration interval)
    : Metric(name_metric, interval)
    { }

void Metrics::HttpRequestMetric::update(void) {
    unsigned int rps = counter_rps.exchange(0, std::memory_order_relaxed);
    
    {
        std::lock_guard<std::mutex> lock(mtx);
        metric_value = std::to_string(rps);
    }
}


// Class MetricManager for management metrics in other thread;
Metrics::MetricManager::MetricManager(std::vector<std::shared_ptr<Metric>>&& metrics)
    : metrics(std::move(metrics))
    { }

Metrics::MetricManager::~MetricManager() {
    stop();
}

void Metrics::MetricManager::add_metric(std::shared_ptr<Metric> metric) {
    metrics.push_back(std::move(metric));
}

void Metrics::MetricManager::start_loop(void) {
    while (running) {
        for (size_t i = 0; i < metrics.size(); i++) {
            metrics[i]->try_update();
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void Metrics::MetricManager::write_loop(const std::string& filename, std::chrono::milliseconds interval) {
    std::ofstream ofs(filename, std::ios::app);
    if (!ofs.is_open()) {
        std::cerr << "Error: can not open file " << filename << " for writing metrics\n";
        return;
    }
    while (running) {
        std::this_thread::sleep_for(interval);

        std::string report = collect();
        ofs << report;
        ofs.flush();
    }
}

void Metrics::MetricManager::run(const std::string& filename, std::chrono::milliseconds interval) {
    this->filename = filename;
    th = std::thread([this]() { start_loop(); });
    write_th = std::thread([this, interval]() { 
        write_loop(this->filename, interval);  
    });
}

std::string Metrics::MetricManager::collect() {
    std::string report;
    for (size_t i = 0; i < metrics.size(); i++) {
        report += metrics[i]->collect() + " | ";
    }
    return report + "\n";
}

void Metrics::MetricManager::stop(void) {
    running = false;
    if (th.joinable())
        th.join();
    if (write_th.joinable())
        write_th.join();
}
