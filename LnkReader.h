
#include <iostream>
#include <fstream>
#include <string>

#define         LNK_SEEK_BEG            std::ios::beg
#define         LNK_SEEK_END            std::ios::end
#define         LNK_SEEK_CUR            std::ios::cur

class LnkReader {
public:

    typedef struct _tagLinkFileHeader {
        DWORD       HeaderSize;     //�ļ�ͷ�Ĵ�С
        GUID        LinkCLSID;      //CLSID
        DWORD       Flags;          //lnk�ļ���־
        DWORD       FileAttributes; //�ļ�����(Ŀ���ļ���)
        FILETIME    CreationTime;   //����ʱ��(Ŀ���ļ���)
        FILETIME    AccessTime;     //����ʱ��
        FILETIME    WriteTime;      //�޸�ʱ��
        DWORD       FileSize;       //�ļ���С
        DWORD       IconIndex;
        DWORD       ShowCommand;    //��ʾ��ʽ
        WORD        Hotkey;         //�ȼ�
        BYTE        retain[10];     //������10byte����
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
            // ����������Ҫ�����ر��ļ���
            lnkFile.close();
        }
    }

    std::string get(std::string&& lnkpath)
    {
        if (lnkFile.is_open())
            lnkFile.close();

        lnkFile.open(lnkpath, std::ios::in | std::ios::binary);     //�Զ�����ֻ���ķ�ʽ���ļ���
        if (!lnkFile.is_open())
            return "Failed to open file.";      //��ʧ��,����-1

        //��ȡ�ļ���С
        seek(LNK_SEEK_END, 0);  //������������λ��
        tagFileSize = tell();   //ͨ��tell��ȡ�ļ���С
        seek(LNK_SEEK_BEG, 0);   //��������ͷ
        //��Ϊlnk�ļ�һ�㶼�Ƚ�С�ʿ��������Ϸ�����ȡ�ļ���С
        
        //��ȡ4���ֽ��ж��Ƿ���lnk�����ļ�
        read(&lnkHeader.HeaderSize, 4);
        if (lnkHeader.HeaderSize != 0x4c)//������lnk�ļ�
        {
            return "Not lnk file.";
            lnkFile.close();
        }
        
        //�ƶ����ݵ��ļ���ͷ��λ��, ��Ϊ��ȡ�ļ�ͷ4���ֽ�ʱ�ı����ļ���ȡ��λ��
        seek(LNK_SEEK_BEG, 0);

        //��ȡ�ļ�ͷ
        read(&lnkHeader, sizeof(LINKFILE_HEADER));

        //��ȡ��ݷ�ʽָ����ļ�·��
        return getLinkTargetIDList();
    }

private:
    std::fstream lnkFile;    //�ļ���
    LONGLONG tagFileSize;   //�ļ���С
    LINKFILE_HEADER lnkHeader;  //lnk�ļ�ͷ


    //��������
    void seek(int pot, int offset)
    {
        lnkFile.seekg(offset, pot);
    }
    //��ȡָ�뵱ǰ��λ��
    LONGLONG tell()
    {
        return lnkFile.tellg();
    }
    //��ȡ����
    void read(PVOID pData, int szCount)
    {
        if (pData == nullptr)
            seek(LNK_SEEK_CUR, szCount);    //�����ֽ�
        else
            lnkFile.read((char*)pData, szCount);
    }
    //��ʼ���ڴ�
    void memzero(PVOID pDst, int iSize)
    {
        ZeroMemory(pDst, iSize);
    }
    //���������л�ȡһ���������ַ���, �����ַ�����(����������)
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

        //��ȡIDList��С
        read(&szIDList, 2);

        for (int szCurrent = 0; szCurrent < szIDList - 2;)
        {
            ITEMID ItemID;

            read(&ItemID.wSize, 2);      //��ȡ��С
            if (ItemID.wSize == 0)       //�ж��Ƿ��� TerminallID
                break;

            read(&ItemID.bType, 1); //��ȡ����

            //�ж�ItemID����
            switch (ItemID.getListType())
            {
            case ITEM_TYPE::ROOT:
            {
                //����
                read(nullptr, ItemID.wSize - 3);

                break;
            }
            case ITEM_TYPE::VOLUME:
            {
                //��ȡ�̷�
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

                read(nullptr, 1);  //����δ֪�ֽ�
                /* ע:read������һ��������NULL��ʾ������� szCount ���ֽ�, �൱�� seek(LNK_SEEK_CUR, szCount) ,���Կ���read��������ϸ��*/
                read(&iFileSize, 4);  //��ȡ�ļ���С(��ʱ����Ҫ)

                //��ȡ�ļ�(�ļ���)�������ں�ʱ��
                read(&DosData, 2);
                read(&DosTime, 2);

                read(&FileAttributes, 2);           //��ȡ�ļ�(�ļ���)����
                int reads = getstring(str);         //��ȡ�ļ�����

                path += str + R"(\)";

                //read(nullptr, ItemID.wSize - 1 - 4 - 2 - 2 - 2 - reads - 3)
                read(nullptr, ItemID.wSize - reads - 14);   //��ʱ����Ҫ������Ϣ

                break;
            }}

            szCurrent += ItemID.wSize;
        }

        if (buf != nullptr)
        {
            delete[] buf;
            buf = nullptr;
        }

        path = path.substr(0, path.size() - 1); //ɾȥ���һ�������"\"
        return path;
    }


};