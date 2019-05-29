import numpy as np
from PIL import Image
import os
import time

os.environ['GLOG_minloglevel'] = '2'
os.environ['CUDA_VISIBLE_DEVICES'] = '0'

import caffe
caffe.set_device(0)
caffe.set_mode_gpu()


batch_list = [1,2,4,8,16,32,64,128,256]


# load network
model_name = 'alexnet'
net = caffe.Net(model_name + '/deploy.prototxt', 
                '{}/{}.caffemodel'.format(model_name, model_name),
                caffe.TEST)

# prepare transformer
transformer = caffe.io.Transformer({'data': net.blobs['data'].data.shape})
transformer.set_mean('data', np.load('ilsvrc_2012_mean.npy').mean(1).mean(1))
transformer.set_transpose('data', (2,0,1))
transformer.set_channel_swap('data', (2,1,0))
transformer.set_raw_scale('data', 255.0)


for batch in batch_list:

  # load images
  net.blobs['data'].reshape(batch,3,224,224)
  for i in range(batch):
    im = caffe.io.load_image('cat.jpg')
    net.blobs['data'].data[i,:,:,:] = transformer.preprocess('data', im)

  # forwarding test
  count_time = 1000
  times = []
  for i in range(count_time):
    start = time.time()
    out = net.forward()
    end = time.time()
    times += [float(end - start) * 1000]
  times = np.array(times, np.float64)
  avg = np.mean(times)
  var = np.var(times)
  print('model: {}, batch: {}, avg: {} ms, var: {}'.format(model_name, batch, avg, var))