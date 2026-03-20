#include "MedianFilter.h"

void mfInit(MedianFilter *mf)
{
    mf->head   = 0;
    mf->count  = 0;
    mf->median = 0.0f;

    for (uint8_t i = 0; i < MF_MAX_WINDOW; i++)
        mf->buffer[i] = 0.0f;
}

/** Simple insertion sort into a temporary array and pick the middle element. */
static float computeMedian(const float *buf, uint8_t n)
{
    float sorted[MF_MAX_WINDOW];
    for (uint8_t i = 0; i < n; i++)
        sorted[i] = buf[i];

    // Insertion sort (n <= 9, perfectly fine)
    for (uint8_t i = 1; i < n; i++)
    {
        float key = sorted[i];
        int8_t j  = (int8_t)i - 1;
        while (j >= 0 && sorted[j] > key)
        {
            sorted[j + 1] = sorted[j];
            j--;
        }
        sorted[j + 1] = key;
    }

    return sorted[n / 2];
}

float mfUpdate(MedianFilter *mf, float value)
{
    mf->buffer[mf->head] = value;
    mf->head = (mf->head + 1) % mf->windowSize;

    if (mf->count < mf->windowSize)
        mf->count++;

    mf->median = computeMedian(mf->buffer, mf->count);
    return mf->median;
}

float mfGetMedian(const MedianFilter *mf)
{
    return mf->median;
}
