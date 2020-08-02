#ifndef agc_hpp
#define agc_hpp

#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>

const float DEFAULT_KERNEL_SIZE = 0.03; // 滤波强度
const float DEFAULT_V = 0.68; // 默认标准明度，各个像素以该值为标准靠近
const float RESTRAIN_HIGHLIGHT = 3; // 抑制高光的强度，设置太大容易导致全局过暗
const float DARK_EDGE = -0.4; // 与标准明度差值小于这个值则认为是太暗部分，使用RGB混合减少噪音

class ParallelMix : public cv::ParallelLoopBody{
public:
    ParallelMix(cv::Mat& _v, cv::Mat& _v_i, cv::Mat& _src, const float _mean_v): v(_v), v_i(_v_i), src(_src), mean_v(_mean_v){}
    void operator()(const cv::Range& range) const{
        for (int i = range.start; i < range.end; ++i){
            uchar *vp = v.ptr<uchar>(i);
            uchar *ip = v_i.ptr<uchar>(i);
            cv::Vec3b *srcp = src.ptr<cv::Vec3b>(i);

            for (int j = 0; j < src.cols; j++) {
                float dealta = ip[j]/255.0 - mean_v;
                float r = dealta / mean_v;
                if(dealta > 0){
                    // restrain highlight
                    r *= powf((1 + dealta), RESTRAIN_HIGHLIGHT);
                }
                float gamma = powf(2 , r );
                if(dealta > DARK_EDGE){
                    float x = powf(vp[j]/255.0  , gamma) * 255;
                    vp[j] =  x;
                    srcp[j] = cv::Vec3b(0, 0, 0);
                }else{
                    // to reduce noise
                    float dd = -0.4 - r;
                    float bx = powf(srcp[j][0]/255.0, gamma+dd/5) * 255;
                    float gx = powf(srcp[j][1]/255.0, gamma+dd/5) * 255;
                    float rx = powf(srcp[j][2]/255.0, gamma+dd/5) * 255;
                    srcp[j][0] = bx * dd;
                    srcp[j][1] = gx * dd;
                    srcp[j][2] = rx * dd;
                    vp[j] = powf(vp[j]/255.0, gamma) * (1 - dd) * 255;
                }
            }
            
        }
      }
private:
    cv::Mat& v;
    cv::Mat& v_i;
    cv::Mat& src;
    float mean_v;
};

class ParallelBGR : public cv::ParallelLoopBody{
public:
    ParallelBGR(cv::Mat& _v_i, cv::Mat& _src, const float _mean_v, const float _mean_d): v_i(_v_i), src(_src), mean_v(_mean_v), mean_d(_mean_d){}
    void operator()(const cv::Range& range) const{
        for (int i = range.start; i < range.end; ++i){
            uchar *ip = v_i.ptr<uchar>(i);
            cv::Vec3b *srcp = src.ptr<cv::Vec3b>(i);
            for (int j = 0; j < src.cols; j++) {
                float dealta = ip[j]/255.0 - mean_v;
                float r = dealta / mean_v;
                float gamma = powf(2 , r + mean_d);
                float bx = powf(srcp[j][0]/255.0, gamma) * 255;
                float gx = powf(srcp[j][1]/255.0, gamma) * 255;
                float rx = powf(srcp[j][2]/255.0, gamma) * 255;
                srcp[j][0] = bx;
                srcp[j][1] = gx;
                srcp[j][2] = rx;
            }
        }
    }
private:
    cv::Mat& v_i;
    cv::Mat& src;
    float mean_v;
    float mean_d;
};


/// Adaptive Local Gamma Correction based on mean value adjustment
/// @param src  source image(BGR)
/// @return destination image(BGR)
static cv::Mat agc_mean_mix(cv::Mat& src, int kernel_size = -1, float mean_v = DEFAULT_V) {
    if(src.empty()) return src;
    
    cv::Mat src_hsv;
    std::vector<cv::Mat> hsv_channels(3);

    cv::cvtColor(src, src_hsv, cv::COLOR_BGR2HSV);
    cv::split(src_hsv, hsv_channels);
    cv::Mat& v = hsv_channels[2];
    
    // light component of V
    cv::Mat v_i;
    kernel_size = kernel_size == -1 ? std::min(src.rows, src.cols) * DEFAULT_KERNEL_SIZE : kernel_size;
    kernel_size = kernel_size % 2 ? kernel_size : kernel_size -1 ;
    // mean filtering replaces time-consuming gaussian filtering
    cv::blur(v, v_i,  cv::Size(kernel_size, kernel_size));
    
    float mean_c = cv::mean(v_i)[0] / 255.0;
    float mean_d = mean_c - mean_v;
    if(mean_d > DARK_EDGE ){
        cv::parallel_for_(cv::Range(0, src.rows), ParallelMix(v, v_i, src, mean_v));
        
        cv::Mat dst_hsv;
        cv::merge(hsv_channels, dst_hsv);
        cv::cvtColor(dst_hsv, dst_hsv, cv::COLOR_HSV2BGR);
        
        cv::Mat dst;
        cv::add(dst_hsv, src, dst);

        return dst;
    }else{
        // brightness too low global use bgr color
        cv::parallel_for_(cv::Range(0, src.rows), ParallelBGR(v_i, src, mean_v, mean_d));
        return src;
    }
}
#endif
