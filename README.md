# idCreator
idCreator是我们设计并且开发一个分布式的id生成器。它主要为业务系统提供唯一、索引友好、
可排序的id。它解决了互联网行业中，使用int自增id或者是string类型的自定义id而导致的
无法方便的分库分表或者是id排序不友好问题。提到id生成器，twitter的snowflake算法
不得不被提起，但是snowflake的算法因为使用了二进制的移位做法，导致其生成的算法过
度的对于机器友好，对于人类并不是那么的友好，也就是说snowflake生成的id对于人类来
说并不能“望文生义”。  

我们目前的idCreator主要提供3种规则的id：  
1. 如果你喜欢snowflake算法，你一样可以选择我们的idCreator，它也有算法支持；  
2. 如果你需要做数据路由，也就是分库分表操作，我们的idCreator就是生成这种带有
路由信息性质的id的，你更应该选择它；  
3. 如果你仅仅自是想做排序id，那么我们的idCreator为你提供了每秒递增的id；  
再不久的将来，目前已经加入了我们的计划。我们将再提供一种id：  
4. 按照时间和自定义步长严格递增的id，主要用来作为状态值等使用。也可以用来做强类型
的数据路由。  

# idCreator安装方法  
1. 下载并且安装libev  
2. 下载当前idCreator源码  
3. 进入idcreator目录，执行make即可  
4. make执行完成后，在idcreator目录下，有idCreator可执行文件  
5. 进入config文件夹，根据你实际的情况配置id生成器信息  
5. 执行./idCreator config/idcreator.config 即可运行  

# idCreator 限制  
1. 目前每台机器一秒大概可以生成10k的id  
2. 目前id生成器的机器总数被限定在10台，mid从0--9  
3. 目前id生成器被设计成明确生成类型的最大值为100，也就是为100种数据生成id  
4. 分库的位被设计成空余了2位，也就是说最多支持一个业务被拆分成100个数据库，从0-99  
5. 目前，id生成器只支持linux运行  

# idCreator客户端  
1. C，本身idCreator就是c写的，所以它有c的客户端没有什么意外  
2. java，但是目前该客户端需要和albianj一起被使用  
3. http，屏蔽各种语言，idCreator支持通过http访问，其内置有web服务器,
expl:http://10.97.19.58:8988/?type=1 可以使用这样的路径访问id生成器，你会得到一串
json的响应：{error:0, errorMessage:success, result:4159860000002400},其中result就是你需要的id值  

详细文档请查看[wiki](https://github.com/crosg/idCreator/wiki)  

# idCreator 文档列表和qq群:  
[idCreator的设计思路文档] (http://www.94geek.com/2015/idcreator.html "idCreator设计思路文档")  
qq群：528658887  

# 关于我们：  

我们是阅文集团的技术团队，阅文集团于2015年成立，统一管理和运营原本属于盛大文学
和腾讯文学旗下的起点中文网、创世中文网、小说阅读网、潇湘书院、红袖添香、云起书
院、榕树下、QQ阅读、中智博文、华文天下等网文品牌。  


