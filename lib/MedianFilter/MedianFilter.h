/**
 * @file MedianFilter.h
 * @brief Sliding-window median filter for removing spike noise ("salt & pepper").
 *
 * Maintains a circular buffer of the last N samples and returns the median
 * value on each update. The median is immune to single-sample outliers.
 *
 *  Function              | Caller  | Notes
 *  --------------------- | ------- | ------------------------------------
 *  mfInit()              | setup   | reset buffer and count
 *  mfUpdate()            | Task 2  | feed new sample, returns median
 *  mfGetMedian()         | Task 3  | last computed median value
 */

#ifndef MEDIAN_FILTER_H
#define MEDIAN_FILTER_H

#include <Arduino.h>

/** Maximum supported window size. */
#define MF_MAX_WINDOW  9

/** Configuration and runtime state for one median filter channel. */
typedef struct
{
    /* --- configuration (set by caller) --------------------------------- */
    uint8_t  windowSize;    /**< Number of samples in the sliding window (odd recommended). */

    /* --- internal state (owned by module) ------------------------------ */
    float    buffer[MF_MAX_WINDOW]; /**< Circular sample buffer.           */
    uint8_t  head;                  /**< Next write position.              */
    uint8_t  count;                 /**< Samples collected so far.         */
    float    median;                /**< Last computed median.             */
} MedianFilter;

/** @brief Reset buffer and sample count. Call once after setting windowSize. */
void mfInit(MedianFilter *mf);

/** @brief Feed a new sample. Returns the current median.
 *  Until the buffer is full, returns the median of available samples. */
float mfUpdate(MedianFilter *mf, float value);

/** @brief Returns the last computed median value. */
float mfGetMedian(const MedianFilter *mf);

#endif
