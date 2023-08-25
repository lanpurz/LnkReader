# LnkReader
windows下获取.lnk文件的路径
使用方法
1.LnkReader类中有两个公开的方法:run和getPath
先向run方法里传入指向一个快捷方式文件(.lnk)路径的字符串(例如:C:\Users\User1\Desktop\App1.lnk)
2.若run的返回值等于0,则可以使用getPath方法获取lnk的目标文件路径

run方法返回-1说明文件打开失败
返回-2说明不是.lnk文件
