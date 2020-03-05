#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc/imgproc.hpp>

class ParallelMix : public cv::ParallelLoopBody{
public:
    ParallelMix(cv::Mat& _v, cv::Mat& _v_i, cv::Mat& _src, const float _average): v(_v), v_i(_v_i), src(_src), average(_average){}
    void operator()(const cv::Range& range) const{
        for (int i = range.start; i < range.end; ++i){
            float *vp = v.ptr<float>(i);
            float *ip = v_i.ptr<float>(i);
            cv::Vec3f *srcp = src.ptr<cv::Vec3f>(i);
            
            for (int j = 0; j < src.cols; j++) {
                float dealta = ip[j] - average;
                float r = dealta / average;
                if(dealta > 0){
                    // inhibition of highlights
                    r *= powf((1 + dealta), 3);
                }
                float gamma = powf(2 , r );
                if(dealta > -0.4){
                    float x = powf(vp[j]  , gamma);
                    vp[j] =  x;
                    srcp[j] = cv::Vec3f(0, 0, 0);
                }else{
                    // to reduce noise
                    float dd = -0.4 - r;
                    float bx = powf(srcp[j][0], gamma+dd/5);
                    float gx = powf(srcp[j][1], gamma+dd/5);
                    float rx = powf(srcp[j][2], gamma+dd/5);
                    srcp[j][0] = bx * dd;
                    srcp[j][1] = gx * dd;
                    srcp[j][2] = rx * dd;
                    vp[j] = powf(vp[j]  , gamma) * (1 - dd);
                }
            }
        }
      }
private:
    cv::Mat& v;
    cv::Mat& v_i;
    cv::Mat& src;
    float average;
};

class ParallelBGR : public cv::ParallelLoopBody{
public:
    ParallelBGR(cv::Mat& _v_i, cv::Mat& _src, const float _average, const float _e): v_i(_v_i), src(_src), average(_average), e(_e){}
    void operator()(const cv::Range& range) const{
        for (int i = range.start; i < range.end; ++i){
            float *ip = v_i.ptr<float>(i);
            cv::Vec3f *srcp = src.ptr<cv::Vec3f>(i);
            for (int j = 0; j < src.cols; j++) {
                float dealta = ip[j] - average;
                float r = dealta / average;
                float gamma = powf(2 , r + e);
                float bx = powf(srcp[j][0], gamma);
                float gx = powf(srcp[j][1], gamma);
                float rx = powf(srcp[j][2], gamma);
                srcp[j][0] = bx;
                srcp[j][1] = gx;
                srcp[j][2] = rx;
            }
        }
    }
private:
    cv::Mat& v_i;
    cv::Mat& src;
    float average;
    float e;
};


//Adaptive Local Gamma Correction based on mean value adjustment
cv::Mat agc_mean_mix(cv::Mat& src, int kernel_size = -1, float average = 0.68) {
    cv::Mat src_hsv;
    std::vector<cv::Mat> hsv_channels(3);
    
    src.convertTo(src, CV_32FC3, 1 / 255.0);
    cv::cvtColor(src, src_hsv, cv::COLOR_BGR2HSV);
    cv::split(src_hsv, hsv_channels);
    cv::Mat& v = hsv_channels[2];
    
    // light component of V
    cv::Mat v_i;
    kernel_size = kernel_size == -1 ? std::min(src.rows, src.cols) / 30 : kernel_size;
    kernel_size = kernel_size % 2 ? kernel_size : kernel_size -1 ;
    // mean filtering replaces time-consuming gaussian filtering
    cv::blur(v, v_i,  cv::Size(kernel_size, kernel_size));
    
    
    float mean = cv::mean(v_i)[0];
    float e = mean - average;
    if(e > -0.4){
        cv::parallel_for_(cv::Range(0, src.rows), ParallelMix(v, v_i, src, average));
        
        cv::Mat dst_hsv;
        cv::merge(hsv_channels, dst_hsv);
        cv::cvtColor(dst_hsv, dst_hsv, cv::COLOR_HSV2BGR);
        dst_hsv.convertTo(dst_hsv, CV_8UC3, 255);
        
        cv::Mat dst_rgb;
        src.convertTo(dst_rgb, CV_8UC3, 255);
        
        cv::Mat dst;
        cv::add(dst_hsv, dst_rgb, dst);

        return dst;
    }else{
        // brightness too low global use bgr color
        cv::parallel_for_(cv::Range(0, src.rows), ParallelBGR(v_i, src, average, e));
        cv::Mat dst;
        src.convertTo(dst, CV_8UC3, 255);
        return dst;      
    }
    
}
