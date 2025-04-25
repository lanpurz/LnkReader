
#include <iostream>
#include <fstream>
#include <string>

#define         LNK_SEEK_BEG            std::ios::beg
#define         LNK_SEEK_END            std::ios::end
#define         LNK_SEEK_CUR            std::ios::cur

class LnkReader {
public:

    typedef struct _tagLinkFileHeader {
        DWORD       HeaderSize;     //文件头的大小
        GUID        LinkCLSID;      //CLSID
        DWORD       Flags;          //lnk文件标志
        DWORD       FileAttributes; //文件属性(目标文件的)
        FILETIME    CreationTime;   //创建时间(目标文件的)
        FILETIME    AccessTime;     //访问时间
        FILETIME    WriteTime;      //修改时间
        DWORD       FileSize;       //文件大小
        DWORD       IconIndex;
        DWORD       ShowCommand;    //显示方式
        WORD        Hotkey;         //热键
        BYTE        retain[10];     //保留的10byte数据
    }LINKFILE_HEADER, * LPLINKFILE_HEADER;

    typedef struct _tagLinkItemID {
        WORD        wSize;
        BYTE        bType;
        BYTE* bData;

        inline int getTypeData() { return (bType & 0xFL); };
        inline int getListType() { return ((bType & 0xF0L) >> 4); };

    }ITEMID, * LPITEMID;

    typedef struct _tagItemType {
        static const BYTE      ROOT = 1;
        static const BYTE      VOLUME = 2;
        static const BYTE      FILE = 3;
    }ITEM_TYPE;

public:

    LnkReader() = default;
    virtual ~LnkReader()
    {
        if (lnkFile.is_open())
        {
            // 析构函数主要用来关闭文件流
            lnkFile.close();
        }
    }

    std::string get(std::string&& lnkpath)
    {
        if (lnkFile.is_open())
            lnkFile.close();

        lnkFile.open(lnkpath, std::ios::in | std::ios::binary);     //以二进制只读的方式打开文件流
        if (!lnkFile.is_open())
            return "Failed to open file.";      //打开失败,返回-1

        //获取文件大小
        seek(LNK_SEEK_END, 0);  //先跳到结束的位置
        tagFileSize = tell();   //通过tell获取文件大小
        seek(LNK_SEEK_BEG, 0);   //再跳到开头
        //因为lnk文件一般都比较小故可以用以上方法获取文件大小
        
        //读取4个字节判断是否是lnk类型文件
        read(&lnkHeader.HeaderSize, 4);
        if (lnkHeader.HeaderSize != 0x4c)//若不是lnk文件
        {
            return "Not lnk file.";
            lnkFile.close();
        }
        
        //移动数据到文件开头的位置, 因为读取文件头4个字节时改变了文件读取的位置
        seek(LNK_SEEK_BEG, 0);

        //读取文件头
        read(&lnkHeader, sizeof(LINKFILE_HEADER));

        //获取快捷方式指向的文件路径
        return getLinkTargetIDList();
    }

private:
    std::fstream lnkFile;    //文件流
    LONGLONG tagFileSize;   //文件大小
    LINKFILE_HEADER lnkHeader;  //lnk文件头


    //跳过数据
    void seek(int pot, int offset)
    {
        lnkFile.seekg(offset, pot);
    }
    //获取指针当前的位置
    LONGLONG tell()
    {
        return lnkFile.tellg();
    }
    //读取数据
    void read(PVOID pData, int szCount)
    {
        if (pData == nullptr)
            seek(LNK_SEEK_CUR, szCount);    //跳过字节
        else
            lnkFile.read((char*)pData, szCount);
    }
    //初始化内存
    void memzero(PVOID pDst, int iSize)
    {
        ZeroMemory(pDst, iSize);
    }
    //从数据流中获取一个完整的字符串, 返回字符长度(包括结束符)
    int getstring(std::string& src)
    {
        src = "";
        int n = 0;
        for (char ch[2] = "";; n++)
        {
            read(ch, 1);
            if (ch[0] == '\0')
                break;

            src += std::string(ch);
        }

        return n + 1;
    }

    std::string getLinkTargetIDList()
    {

        int szIDList = 0;
        char* buf = nullptr;
        std::string path = "";

        //读取IDList大小
        read(&szIDList, 2);

        for (int szCurrent = 0; szCurrent < szIDList - 2;)
        {
            ITEMID ItemID;

            read(&ItemID.wSize, 2);      //读取大小
            if (ItemID.wSize == 0)       //判断是否是 TerminallID
                break;

            read(&ItemID.bType, 1); //读取类型

            //判断ItemID类型
            switch (ItemID.getListType())
            {
            case ITEM_TYPE::ROOT:
            {
                //忽略
                read(nullptr, ItemID.wSize - 3);

                break;
            }
            case ITEM_TYPE::VOLUME:
            {
                //读取盘符
                char* buf = new char[ItemID.wSize - 3];
                memzero(buf, ItemID.wSize - 3);

                read(buf, ItemID.wSize - 3);

                path += std::string(buf);

                break;
            }
            case ITEM_TYPE::FILE:
            {
                int iFileSize = 0;
                WORD DosData = 0, DosTime = 0, FileAttributes = 0;
                std::string str = "";

                read(nullptr, 1);  //跳过未知字节
                /* 注:read函数第一个参数填NULL表示向后跳过 szCount 个字节, 相当于 seek(LNK_SEEK_CUR, szCount) ,可以看看read函数具体细节*/
                read(&iFileSize, 4);  //读取文件大小(暂时不需要)

                //读取文件(文件夹)创建日期和时间
                read(&DosData, 2);
                read(&DosTime, 2);

                read(&FileAttributes, 2);           //读取文件(文件夹)类型
                int reads = getstring(str);         //读取文件名称

                path += str + R"(\)";

                //read(nullptr, ItemID.wSize - 1 - 4 - 2 - 2 - 2 - reads - 3)
                read(nullptr, ItemID.wSize - reads - 14);   //暂时不需要额外信息

                break;
            }}

            szCurrent += ItemID.wSize;
        }

        if (buf != nullptr)
        {
            delete[] buf;
            buf = nullptr;
        }

        path = path.substr(0, path.size() - 1); //删去最后一个多余的"\"
        return path;
    }


};