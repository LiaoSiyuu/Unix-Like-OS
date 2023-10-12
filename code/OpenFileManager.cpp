#include "_Kernel.h"
InodeTable::InodeTable()
{

}

InodeTable::~InodeTable()
{

}

Inode* InodeTable::IGet(int inumber)
{
    Inode* pInode;
    _Kernel* k = _Kernel::getInstance();
    BufferManager* bufMgr = _Kernel::getInstance()->gBufferManager();
    while (true)
    {
        int index = this->IsLoaded(inumber);
        if (index >= 0)	/* �ҵ��ڴ濽�� */
        {
            pInode = &(this->m_Inode[index]);
            pInode->i_count++;
            return pInode;
        }
        else	/* û��Inode���ڴ濽���������һ�������ڴ�Inode */
        {
            pInode = this->GetFreeInode();
            if (pInode == NULL)
                return NULL;
            else
            {
                pInode->i_number = inumber;
                pInode->i_count++;
                Buf* pBuf = bufMgr->Bread(FileSystem::INODE_ZONE_START_SECTOR + inumber / FileSystem::INODE_NUMBER_PER_SECTOR);
                pInode->ICopy(pBuf, inumber);
                bufMgr->Brelse(pBuf);
                return pInode;
            }
        }
    }
    return NULL;
}

void InodeTable::IPut(Inode *pNode)
{
    FileSystem* fileSys = _Kernel::getInstance()->gFileSystem();
    if (pNode->i_count == 1)
    {
        if (pNode->i_nlink <= 0)
        {
            pNode->ITrunc();
            pNode->i_mode = 0;
            fileSys->IFree(pNode->i_number);
        }

        pNode->IUpdate();
        pNode->i_flag = 0;
        pNode->i_number = -1;
    }
    pNode->i_count--;
}

void InodeTable::UpdateInodeTable()
{
    for (int i = 0; i < InodeTable::NINODE; i++)
    {
        if (this->m_Inode[i].i_count != 0)
            this->m_Inode[i].IUpdate();
    }
}

int InodeTable::IsLoaded(int inumber)
{
    for (int i = 0; i < InodeTable::NINODE; i++)
    {
        if (this->m_Inode[i].i_number == inumber && this->m_Inode[i].i_count != 0)
            return i;
    }
    return -1;
}

Inode* InodeTable::GetFreeInode()
{
    for (int i = 0; i < InodeTable::NINODE; i++)
    {
        if (this->m_Inode[i].i_count == 0)
            return &(this->m_Inode[i]);
    }
    return NULL;
}

File::File()
{
    this->f_count = 0;
    this->f_flag = 0;
    this->f_offset = 0;
    this->f_inode = NULL;
}

File::~File()
{

}

File* OpenFileTable::FAlloc()
{
    int fd;
    _Kernel* k = _Kernel::getInstance();

    /* ���ں˴��ļ����������л�ȡһ�������� */
    fd = k->gOpenFiles()->AllocFreeSlot();

    if (fd < 0)
    {
        return NULL;
    }

    for (int i = 0; i < OpenFileTable::NFILE; i++)
    {
        if (this->m_File[i].f_count == 0)
        {
            /* ������������File�ṹ�Ĺ�����ϵ */
            k->gOpenFiles()->SetF(fd, &this->m_File[i]);
            /* ���Ӷ�file�ṹ�����ü��� */
            this->m_File[i].f_count++;
            /* ����ļ�����дλ�� */
            this->m_File[i].f_offset = 0;
            return (&this->m_File[i]);
        }
    }
    return NULL;
}

void OpenFileTable::CloseF(File *pFile)
{
    /* ���ü���f_count����Ϊ0�����ͷ�File�ṹ */
    if (pFile->f_count <= 1)
        _Kernel::getInstance()->gInodeTable()->IPut(pFile->f_inode);

    /* ���õ�ǰFile�Ľ�������1 */
    pFile->f_count--;
}

OpenFiles::OpenFiles()
{
    for (int i = 0; i < OpenFiles::NOFILES; i++)
        SetF(i, NULL);
}

int OpenFiles::AllocFreeSlot()
{
    int i;
    for (i = 0; i < OpenFiles::NOFILES; i++)
    {
        if (this->k_OpenFileTable[i] == NULL)
        {
            _Kernel::getInstance()->callReturn = i;
            return i;
        }
    }
    _Kernel::getInstance()->callReturn = -1;
    return -1;
}

File* OpenFiles::GetF(int fd)
{
    File* pFile;
    _Kernel* k = _Kernel::getInstance();
    if (fd < 0 || fd >= OpenFiles::NOFILES)
    {
        k->error = _Kernel::BADF;
        return NULL;
    }
    if ((pFile = this->k_OpenFileTable[fd]) == NULL)
    {
        k->error = _Kernel::BADF;
        return NULL;
    }
    return pFile;
}

void OpenFiles::SetF(int fd, File* pFile)
{
    if (fd < 0 || fd >= OpenFiles::NOFILES)
        return;
    this->k_OpenFileTable[fd] = pFile;
}
