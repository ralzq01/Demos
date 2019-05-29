# Inference Code For Caffe

## Installation

* Anaconda Envirionment

```
conda install lmdb
conda install leveldb
conda install caffe-gpu
conda install caffe
```

## Getting Started

### Prerequest

* Model Preparation:

  Visit [Model Zoo](https://github.com/BVLC/caffe/wiki/Model-Zoo#models-used-by-the-vgg-team-in-ilsvrc-2014) to download the `.prototxt` and `.caffemodel`

* Image Preparation:

  Use [Caffe Cat Example](https://github.com/BVLC/caffe/blob/master/examples/images/cat.jpg) as input image

* Transformer Preparation:

  Use [ilsvrc_2012_mean](https://github.com/BVLC/caffe/raw/master/python/caffe/imagenet/ilsvrc_2012_mean.npy) as the transformer.

Put model inside a folder under this directory and name it to `xxxmodel/deploy.prototxt` and `xxxmodel/xxxmodel.caffemodel`.

Put `cat.jpg` and `ilsvrc_2012_mean.npy` under this directory

### Run

modify `inference.py` line 7: for selecting a GPU id; line 14: for selecting batch_size list; line 18-20: for selecting model; Then:

```python
python inference.py
```

The code will run each batch for 500 times and print the average times (miliseconds) per batch.