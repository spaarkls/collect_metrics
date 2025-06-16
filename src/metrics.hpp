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
#include <atomic>

#undef DATETIME_STR_SIZE
#define DATETIME_STR_SIZE 64

/**
 * @file metrics.hpp
 * @brief Библиотека для сбора и асинхронной записи метрик в файл.
 */

namespace Metrics {

/**
 * @brief Константа для перевода дробей в проценты.
 */
inline constexpr double PERCENT_RATIO = 100.0;

/**
 * @brief Глобальный счётчик HTTP-запросов (RPS).
 */
inline std::atomic<unsigned int> counter_rps {};

/**
 * @brief Увеличивает счётчик HTTP-запросов.
 *
 * Вызывать из любого потока, в котором происходит приём HTTP-запроса.
 */
inline void count_http_request(void) {
    counter_rps.fetch_add(1, std::memory_order_release);
}


/**
 * @brief Абстрактный базовый класс для метрик.
 */
class Metric {
protected:
    std::string title_metric; ///< Название метрики.
    std::string metric_value; ///< Последнее значение метрики.
    std::mutex mtx;           ///< Мьютекс для синхронизации доступа.
    std::chrono::steady_clock::time_point last_update; ///< Время последнего обновления.
    std::chrono::steady_clock::duration update_interval; ///< Интервал между обновлениями.
    
public:
    /**
     * @brief Конструктор метрики.
     * @param name_metric Название метрики.
     * @param interval Интервал обновления.
     */
    Metric(const std::string& name_metric, std::chrono::steady_clock::duration interval = std::chrono::seconds(1));
    
    /**
     * @brief Виртуальный деструктор
     */
    virtual ~Metric();
    
    /**
     * @brief Метод обновления значения метрики. Должен быть реализован в подклассе.
     */
    virtual void update(void) = 0;
    
    /**
     * @brief Метод пытается обновить метрику, если прошло достаточно времени.
     */
    virtual void try_update(void);

    /**
     * @brief Метод возвращает текущее значение метрики в виде строки.
     * @return Строка с названием метрики и значением метрики.
     */
    virtual std::string collect(void);
};

/**
 * @brief Метрика текущего времени (дата и время).
 */
class DateMetric : public Metric {

public:
    /**
     * @brief Конструктор.
     * @param name_metric Название метрики.
     * @param interval Интервал обновления.
     */
    DateMetric(const std::string& name_metric, std::chrono::steady_clock::duration interval = std::chrono::seconds(1));

    /**
     * @brief Обновляет текущее значение времени.
     */
    virtual void update(void) override final;

};


/**
 * @brief Метрика загрузки CPU.
 */
class CPUMetric : public Metric {
    struct Load {
        unsigned long idl {}; ///< Idle время.
        unsigned long busy {}; ///< Занятое время.
        unsigned long total {}; ///< Общее время.
    };

    /**
     * @brief Приватный метод, считывает значение CPU из /proc/stat.
     * @return Структуру Load.
     */
    Load read_cpu(void);
    Load prev_load; ///< Предыдущее состояние. 
    int kernels; ///< Количество ядер процессора

public:
    /**
     * @brief Конструктор.
     * @param name_metric Название метрики.
     * @param interval Интервал обновления.
     */
    CPUMetric(const std::string& name_metric, std::chrono::steady_clock::duration interval = std::chrono::seconds(1)); 
    
    /**
     * @brief Вычисляет загрузку CPU с момента прошлого вызова.
     */
    virtual void update(void) override final;
};

/**
 * @brief Метрика количества HTTP-запросов в секунду (RPS).
 */
class HttpRequestMetric : public Metric {
public:
    /**
     * @brief Конструктор.
     * @param name_metric Название метрики.
     * @param interval Интервал обновления.
     */
    HttpRequestMetric(const std::string& name_metric, std::chrono::steady_clock::duration interval = std::chrono::seconds(1));

    /**
     * @brief Обновляет RPS, обнуляя глобальный счётчик.
     */
    virtual void update(void) override final;

};


/**
 * @brief Класс для управления всеми метриками и фоновыми потоками обновления и записи.
 */
class MetricManager {
    std::vector<std::shared_ptr<Metric>> metrics; ///< Список метрик.
    std::thread th;     ///< Поток обновления метрик. 
    std::thread write_th; ///< Поток записи в файл.
    std::string filename; ///< Имя файла для записи.
    std::atomic<bool> running = {true}; ///< Флаг активности.

    /**
     * @brief Цикл обновления метрик.
     */
    void start_loop(void);
    
    /**
     * @brief Цикл записи метрик в файл.
     * @param filename Путь к файлу.
     * @param interval Интервал между записями.
     */
    void write_loop(const std::string& filename, std::chrono::milliseconds interval);

public:
    /**
     * @brief Конструктор с передачей метрик.
     * @param metrics Массив метрик.
     */
    MetricManager(std::vector<std::shared_ptr<Metric>>&& metrics);
    
    /**
     * @brief Конструктор с передачей метрик.
     * @param metrics Массив метрик. 
     */
    MetricManager(const std::vector<std::shared_ptr<Metric>>& metrics);

    /**
     * @brief Конструктор по умолчанию.
     */
    MetricManager() = default;

    /**
     * @brief Деструктор. Останавливает фоновую работу.
     */
    ~MetricManager();
    
    /**
     * @brief Добавляет новую метрику.
     * @param metric Указатель на метрику.
     */
    void add_metric(std::shared_ptr<Metric> metric);

    /**
     * @brief Запускает фоновое обновление и запись.
     * @param filename Имя файла для записи.
     * @param interval Интервал записи.
     */
    void run(const std::string& filename, std::chrono::milliseconds interval = std::chrono::milliseconds(1000));


    /**
     * @brief Останавливает работу.
     */
    void stop(void);


    /**
     * @brief Собирает текущее состояние всех метрик.
     * @return Строка с объединёнными значениями.
     */
    std::string collect(void);
};

}