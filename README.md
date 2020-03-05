# simple_agc
a hpp file use c++
simple adaptive local gamma correction based on mean(or gaussian base Retinex) value adjustment

> 主要参考 博客 [OpenCV图像处理专栏十二 |《基于二维伽马函数的光照不均匀图像自适应校正算法》](https://blog.csdn.net/just_sort/article/details/88569129) 中的代码
> 同时也参考了原论文: 刘志成,王殿伟,刘颖,刘学杰.基于二维伽马函数的光照不均匀图像自适应校正算法[J].北京理工大学学报,2016,36(02):191-196+214.

做了以下定制化内容：
 - 抑制高光（如日光，过曝）
 - 全局过暗全局使用BGR颜色（太暗了，再使用HSV颜色，极易出现彩色噪音，虽然色彩保留较好，但是效果和直接使用HSV颜色模式下V直方图均衡差不多）
 - 局部过暗使用HSV混合BGR（实验发现只使用HSV颜色，在过暗时容易出现彩色噪音，因为第亮度时，只改变V，会对HS产生较大变化，我猜的）
 - 不再使用平均亮度作为gamma校正标准，而是使用给定亮度 
 -  不再使用3尺度加权高斯滤波求光照分量，而是使用一次均值滤波（追求速度，虽然不是基于Retinex 理论，但是发现效果差别不是很大）

优化部分：
 - 使用并行多线程，速度提升一倍
 - 更多基于opencv操作，在i5 mac 上8k图平均1.5s处理完
 - 对过暗和过亮部分做了优化

[实验对比](http://linkstack.net/article/68) 查看
