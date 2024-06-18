# -*- coding: utf-8 -*-
# 引入依赖包
# pip install alibabacloud_facebody20191230

import os
import io
from urllib.request import urlopen
from alibabacloud_facebody20191230.client import Client
from alibabacloud_facebody20191230.models import SearchFaceAdvanceRequest
from alibabacloud_tea_openapi.models import Config
from alibabacloud_tea_util.models import RuntimeOptions

config = Config(
    # 创建AccessKey ID和AccessKey Secret，请参考https://help.aliyun.com/document_detail/175144.html。
    # 如果您用的是RAM用户的AccessKey，还需要为RAM用户授予权限AliyunVIAPIFullAccess，请参考https://help.aliyun.com/document_detail/145025.html。
    # 从环境变量读取配置的AccessKey ID和AccessKey Secret。运行代码示例前必须先配置环境变量。
    access_key_id=os.environ.get('ALIBABA_CLOUD_ACCESS_KEY_ID'),
    access_key_secret=os.environ.get('ALIBABA_CLOUD_ACCESS_KEY_SECRET'),
    # 访问的域名
    endpoint='facebody.cn-shanghai.aliyuncs.com',
    # 访问的域名对应的region
    region_id='cn-shanghai'
)



def alibaba_face():
    search_face_request = SearchFaceAdvanceRequest()
    #场景一：文件在本地
    stream0 = open(r'/tmp/SearchFace.jpg', 'rb')
    search_face_request.image_url_object = stream0

    #场景二：使用任意可访问的url
    # url = 'https://viapi-test-bj.oss-cn-beijing.aliyuncs.com/viapi-3.0domepic/facebody/SearchFace1.png'
    # img = urlopen(url).read()
    # search_face_request.image_url_object = io.BytesIO(img)
    search_face_request.db_name = 'default'
    search_face_request.limit = 5

    runtime_option = RuntimeOptions()
    try:
      # 初始化Client
      client = Client(config)
      # print("line 44 init Client Success!")
      response = client.search_face_advance(search_face_request, runtime_option)
      # 获取整体结果
      
      print(response.body)
      #这行代码使用了列表推导式，它遍历match_list列表中的第一个元素（索引为0）所包含的FaceItems列表。
      # 对于FaceItems列表中的每个项目（假设每个项目都是一个字典），它提取出键为'Score'的值，并将这些值组成一个新的列表
      match_list = response.body.to_map()['Data']['MatchList'] # 拿到所有我们的需要的字典
      # 外面加这个[] --  set集合，无序不重复数据集合 
      scores = [item['Score'] for item in match_list[0]['FaceItems']] # 取到字典中所有的 key= 'Score',的value --> 取到所有的分数(人脸匹配度)
     
          
      highest_score = max(scores)   #  
      value = round(highest_score, 2) #  四舍五入去小数点后两位
      # print(value)
      # print("line 56 获取得到效果 Success!")
      return value
      
    except Exception as error:
      # 获取整体报错信息
      print(error)
      # print("Line 62  有错")
      # 获取单个字段
      print(error.code)
      # tips: 可通过error.__dict__查看属性名称
      # print("Line 66  有错")

#关闭流
#stream0.close()
