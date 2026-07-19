/*
 * Enhanced Performance Counters Implementation
 * Advanced metrics collection and analysis for GNU Mach
 * 
 * Copyright (c) 2024 Cognu Mach Contributors
 * All rights reserved.
 */

#include <kern/perf_counters.h>
#include <kern/kalloc.h>
#include <kern/printf.h>
#include <kern/lock.h>
#include <kern/mach_clock.h>
#include <mach/time_value.h>
#include <string.h>

/*
 * Performance counter names for reporting
 */
static const char *perf_counter_names[] = {
    "CPU Instructions",
    "Cache Misses", 
    "Branch Mispredicts",
    "TLB Misses",
    "Memory Bandwidth",
    "Lock Contention",
    "Interrupt Overhead", 
    "Schedule Latency",
    "IPC Latency",
    "VM Pressure"
};

/*
 * Global performance counter state
 */
static struct {
    perf_counter_data_t counters[PERF_COUNTER_MAX];
    boolean_t enabled[PERF_COUNTER_MAX];
    simple_lock_irq_data_t lock;
    boolean_t initialized;
    uint64_t last_snapshot_time;
    uint32_t snapshot_count;
} perf_state;

/*
 * Historical data for trend analysis (simplified ring buffer)
 */
#define TREND_HISTORY_SIZE 100
static struct {
    uint64_t values[PERF_COUNTER_MAX][TREND_HISTORY_SIZE];
    uint64_t timestamps[TREND_HISTORY_SIZE];
    uint32_t head;
    uint32_t count;
} trend_history;

/*
 * Get high-resolution timestamp
 */
static uint64_t
perf_get_timestamp_us(void)
{
    time_value64_t tv;
    record_time_stamp(&tv);
    return ((uint64_t)tv.seconds * 1000000ULL) + 
           ((uint64_t)tv.nanoseconds / 1000ULL);
}

/*
 * Initialize enhanced performance counters
 */
void
perf_counters_init(void)
{
    int i;
    uint64_t now;
    
    now = perf_get_timestamp_us();
    
    /* Initialize counter data */
    for (i = 0; i < PERF_COUNTER_MAX; i++) {
        perf_state.counters[i].value = 0;
        perf_state.counters[i].max_value = 0;
        perf_state.counters[i].min_value = UINT64_MAX;
        perf_state.counters[i].total = 0;
        perf_state.counters[i].samples = 0;
        perf_state.counters[i].last_update = now;
        perf_state.enabled[i] = TRUE;  /* Enable all by default */
    }
    
    /* Initialize trend history */
    memset(&trend_history, 0, sizeof(trend_history));
    
    simple_lock_init_irq(&perf_state.lock);
    perf_state.initialized = TRUE;
    perf_state.last_snapshot_time = now;
    perf_state.snapshot_count = 0;
    
    printf("Enhanced performance counters initialized (%d counters)\n", 
           PERF_COUNTER_MAX);
}

/*
 * Update a performance counter
 */
void
perf_counter_update(perf_counter_type_t type, uint64_t value)
{
    spl_t s;
    perf_counter_data_t *counter;
    uint64_t now;
    
    if (!perf_state.initialized || type >= PERF_COUNTER_MAX || 
        !perf_state.enabled[type]) {
        return;
    }
    
    now = perf_get_timestamp_us();
    s = simple_lock_irq(&perf_state.lock);
    
    counter = &perf_state.counters[type];
    
    /* Update counter statistics */
    counter->value = value;
    counter->total += value;
    counter->samples++;
    counter->last_update = now;
    
    /* Update min/max */
    if (value > counter->max_value) {
        counter->max_value = value;
    }
    if (value < counter->min_value) {
        counter->min_value = value;
    }
    
    simple_unlock_irq(s, &perf_state.lock);
}

/*
 * Get counter data
 */
boolean_t
perf_counter_get(perf_counter_type_t type, perf_counter_data_t *data)
{
    spl_t s;
    
    if (!perf_state.initialized || type >= PERF_COUNTER_MAX || !data) {
        return FALSE;
    }
    
    s = simple_lock_irq(&perf_state.lock);
    *data = perf_state.counters[type];
    simple_unlock_irq(s, &perf_state.lock);
    
    return TRUE;
}

/*
 * Take system performance snapshot
 */
boolean_t
perf_take_system_snapshot(perf_system_snapshot_t *snapshot)
{
    spl_t s;
    int i;
    uint64_t now;
    
    if (!perf_state.initialized || !snapshot) {
        return FALSE;
    }
    
    now = perf_get_timestamp_us();
    
    s = simple_lock_irq(&perf_state.lock);
    
    /* Basic snapshot data */
    snapshot->timestamp = now;
    snapshot->active_tasks = 10;      /* Simplified - would query task count */
    snapshot->active_threads = 20;    /* Simplified - would query thread count */
    snapshot->total_memory_used = 1024 * 1024 * 64; /* 64MB example */
    snapshot->cpu_utilization = 75;   /* 75% example */
    
    /* Copy all counter data */
    for (i = 0; i < PERF_COUNTER_MAX; i++) {
        snapshot->counters[i] = perf_state.counters[i];
    }
    
    /* CPU time breakdown (simplified) */
    snapshot->cpu_time.user_time = 50000;     /* 50ms */
    snapshot->cpu_time.kernel_time = 30000;   /* 30ms */
    snapshot->cpu_time.idle_time = 20000;     /* 20ms */
    snapshot->cpu_time.interrupt_time = 5000; /* 5ms */
    
    perf_state.last_snapshot_time = now;
    perf_state.snapshot_count++;
    
    /* Add to trend history */
    uint32_t next_head = (trend_history.head + 1) % TREND_HISTORY_SIZE;
    trend_history.timestamps[trend_history.head] = now;
    
    for (i = 0; i < PERF_COUNTER_MAX; i++) {
        trend_history.values[i][trend_history.head] = 
            perf_state.counters[i].value;
    }
    
    trend_history.head = next_head;
    if (trend_history.count < TREND_HISTORY_SIZE) {
        trend_history.count++;
    }
    
    simple_unlock_irq(s, &perf_state.lock);
    
    return TRUE;
}

/*
 * Simple trend analysis (growth rate calculation)
 */
boolean_t
perf_analyze_trends(perf_counter_type_t type, perf_trend_data_t *trend)
{
    spl_t s;
    uint64_t *values;
    uint64_t start_value, end_value, time_diff;
    uint32_t start_idx, end_idx;
    int i, valid_samples;
    uint64_t sum = 0, variance = 0, mean;
    
    if (!perf_state.initialized || type >= PERF_COUNTER_MAX || 
        !trend || trend_history.count < 2) {
        return FALSE;
    }
    
    s = simple_lock_irq(&perf_state.lock);
    
    values = trend_history.values[type];
    
    /* Use last 10 samples or all available samples */
    valid_samples = (trend_history.count < 10) ? trend_history.count : 10;
    end_idx = (trend_history.head - 1 + TREND_HISTORY_SIZE) % TREND_HISTORY_SIZE;
    start_idx = (end_idx - valid_samples + 1 + TREND_HISTORY_SIZE) % TREND_HISTORY_SIZE;
    
    start_value = values[start_idx];
    end_value = values[end_idx];
    time_diff = trend_history.timestamps[end_idx] - 
                trend_history.timestamps[start_idx];
    
    /* Calculate basic trend data */
    trend->counter_type = type;
    trend->trend_period = time_diff;
    
    /* Growth rate (simple linear) */
    if (start_value > 0 && time_diff > 0) {
        trend->growth_rate = ((double)(end_value - start_value) / start_value) * 
                            (1000000.0 / time_diff); /* per second */
    } else {
        trend->growth_rate = 0.0;
    }
    
    /* Calculate mean for volatility */
    for (i = 0; i < valid_samples; i++) {
        uint32_t idx = (start_idx + i) % TREND_HISTORY_SIZE;
        sum += values[idx];
    }
    mean = sum / valid_samples;
    
    /* Calculate variance (simplified) */
    for (i = 0; i < valid_samples; i++) {
        uint32_t idx = (start_idx + i) % TREND_HISTORY_SIZE;
        uint64_t diff = (values[idx] > mean) ? (values[idx] - mean) : 
                                               (mean - values[idx]);
        variance += diff * diff;
    }
    
    trend->volatility = (double)variance / valid_samples;
    
    /* Simple anomaly detection (high volatility or extreme growth rate) */
    trend->anomaly_detected = (trend->volatility > 1000000) || 
                              (trend->growth_rate > 5.0) || 
                              (trend->growth_rate < -5.0);
    
    /* Simple prediction (linear extrapolation) */
    trend->prediction_next = end_value + (uint64_t)(trend->growth_rate * end_value);
    
    simple_unlock_irq(s, &perf_state.lock);
    
    return TRUE;
}

/*
 * Detect performance anomalies
 */
boolean_t
perf_detect_anomalies(perf_system_snapshot_t *snapshots, uint32_t count,
                     perf_counter_type_t **anomalous_counters,
                     uint32_t *anomaly_count)
{
    perf_trend_data_t trend;
    static perf_counter_type_t anomaly_list[PERF_COUNTER_MAX];
    uint32_t anomalies = 0;
    int i;
    
    if (!snapshots || count == 0 || !anomaly_count) {
        return FALSE;
    }
    
    /* Check each counter for anomalies */
    for (i = 0; i < PERF_COUNTER_MAX; i++) {
        if (perf_analyze_trends(i, &trend) && trend.anomaly_detected) {
            if (anomalies < PERF_COUNTER_MAX) {
                anomaly_list[anomalies] = i;
                anomalies++;
            }
        }
    }
    
    *anomalous_counters = (anomalies > 0) ? anomaly_list : NULL;
    *anomaly_count = anomalies;
    
    return TRUE;
}

/*
 * Get performance counter name
 */
const char *
perf_counter_name(perf_counter_type_t type)
{
    if (type >= PERF_COUNTER_MAX) {
        return "Unknown";
    }
    return perf_counter_names[type];
}

/*
 * Reset all performance counters
 */
void
perf_counters_reset(void)
{
    spl_t s;
    int i;
    uint64_t now;
    
    if (!perf_state.initialized) {
        return;
    }
    
    now = perf_get_timestamp_us();
    s = simple_lock_irq(&perf_state.lock);
    
    for (i = 0; i < PERF_COUNTER_MAX; i++) {
        perf_state.counters[i].value = 0;
        perf_state.counters[i].max_value = 0;
        perf_state.counters[i].min_value = UINT64_MAX;
        perf_state.counters[i].total = 0;
        perf_state.counters[i].samples = 0;
        perf_state.counters[i].last_update = now;
    }
    
    /* Reset trend history */
    memset(&trend_history, 0, sizeof(trend_history));
    perf_state.snapshot_count = 0;
    
    simple_unlock_irq(s, &perf_state.lock);
    
    printf("Performance counters reset\n");
}

/*
 * Enable/disable specific counters
 */
boolean_t
perf_counter_enable(perf_counter_type_t type, boolean_t enable)
{
    spl_t s;
    
    if (!perf_state.initialized || type >= PERF_COUNTER_MAX) {
        return FALSE;
    }
    
    s = simple_lock_irq(&perf_state.lock);
    perf_state.enabled[type] = enable;
    simple_unlock_irq(s, &perf_state.lock);
    
    return TRUE;
}

/*
 * Get system performance health score (0-100)
 */
uint32_t
perf_get_system_health_score(void)
{
    spl_t s;
    uint32_t score = 100;  /* Start with perfect score */
    perf_trend_data_t trend;
    int i, penalty;
    
    if (!perf_state.initialized) {
        return 0;
    }
    
    s = simple_lock_irq(&perf_state.lock);
    
    /* Analyze each counter and penalize for anomalies */
    for (i = 0; i < PERF_COUNTER_MAX; i++) {
        if (perf_state.enabled[i] && 
            perf_analyze_trends(i, &trend) && 
            trend.anomaly_detected) {
            
            /* Apply penalties based on severity */
            penalty = 10;  /* Base penalty */
            
            /* Higher penalty for critical counters */
            if (i == PERF_COUNTER_MEMORY_BANDWIDTH || 
                i == PERF_COUNTER_SCHEDULE_LATENCY) {
                penalty = 20;
            }
            
            /* Higher penalty for extreme growth rates */
            if (trend.growth_rate > 10.0 || trend.growth_rate < -10.0) {
                penalty += 10;
            }
            
            score = (score > penalty) ? (score - penalty) : 0;
        }
    }
    
    simple_unlock_irq(s, &perf_state.lock);
    
    return score;
}