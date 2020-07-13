* 终端与特性均完成，与要求一致
* 内置命令包括myexport\myecho\myls\mycd\mypwd
	* myexport是个异类，与真export不同，功能是向PATH添加新路径，示例：myexport /home => export PATH=/home:$PATH
	* myecho blablabla
	* myls  或者  myls pathname
	* mycd pathname
	* mypwd
* 外部自实现命令包括myls\mytouch\mymkdir
	* 使用前，先用myexport pathname将当前路径加入PATH， 再调用即可
	
