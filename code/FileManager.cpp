#include "FileManager.h"
#include "_Kernel.h"
#include "Utility.h"

void FileManager::Open()
{
    Inode* pInode;
    _Kernel* k = _Kernel::getInstance();

    pInode = this->NameI(NextChar, FileManager::OPEN);
    if (pInode == NULL)
        return;
    this->Open1(pInode, k->mode, 0);
}

void FileManager::Creat()
{
    Inode* pInode;
    _Kernel* k = _Kernel::getInstance();
    unsigned int newACCMode = k->mode & (Inode::IRWXU | Inode::IRWXG | Inode::IRWXO);
    pInode = this->NameI(NextChar, FileManager::CREATE);

    if (pInode == NULL)
    {
        if (k->error != _Kernel::NO_ERROR)
            return;
        /* ����Inode */
        pInode = this->MakNode(newACCMode & (~Inode::ISVTX));
        if (pInode == NULL)
            return;
        this->Open1(pInode, File::FWRITE, 2);
    }
    else
    {
        this->Open1(pInode, File::FWRITE, 1);
        pInode->i_mode |= newACCMode;
    }
}

/*
* trf == 0��open����
* trf == 1��creat���ã�creat�ļ���ʱ��������ͬ�ļ������ļ�
* trf == 2��creat���ã�creat�ļ���ʱ��δ������ͬ�ļ������ļ��������ļ�����ʱ��һ������
* mode���������ļ�ģʽ����ʾ�ļ������� ����д���Ƕ�д
*/
void FileManager::Open1(Inode* pInode, int mode, int trf)
{
    _Kernel* k = _Kernel::getInstance();

    if (trf != 2 && (mode & File::FWRITE))
    {
        /* openȥдĿ¼�ļ��ǲ������ */
        if ((pInode->i_mode & Inode::IFMT) == Inode::IFDIR)
            k->error = _Kernel::ISDIR;
    }
    if (k->error != _Kernel::NO_ERROR) {
        k->gInodeTable()->IPut(pInode);
        return;
    }

    /* ��creat�ļ���ʱ��������ͬ�ļ������ļ����ͷŸ��ļ���ռ�ݵ������̿� */
    if (trf == 1)
        pInode->ITrunc();


    /* ������ļ����ƿ�File�ṹ */
    File* pFile = k->gOpenFileTable()->FAlloc();
    if (pFile == NULL)
    {
        k->gInodeTable()->IPut(pInode);
        return;
    }
    /* ���ô��ļ���ʽ������File�ṹ���ڴ�Inode�Ĺ�����ϵ */
    pFile->f_flag = mode & (File::FREAD | File::FWRITE);
    pFile->f_inode = pInode;
    if (trf != 0 && k->isDir)
        pInode->i_mode |= Inode::IFDIR;
    return;
}

Inode* FileManager::MakNode(unsigned int mode)
{
    Inode* pInode;
    _Kernel* k = _Kernel::getInstance();

    /* ����һ������DiskInode������������ȫ����� */
    pInode = k->gFileSystem()->IAlloc();

    if (pInode == NULL)
        return NULL;

    pInode->i_flag |= (Inode::IACC | Inode::IUPD);
    pInode->i_mode = mode | Inode::IALLOC;
    pInode->i_nlink = 1;

    /* ����Ŀ¼����Inode��Ų��� */
    k->dent.inode = pInode->i_number;

    /* ����Ŀ¼����pathname�������� */
    for (int i = 0; i < DirectoryEntry::DIRSIZ; i++)
        k->dent.name[i] = k->dbuf[i];
    k->k_IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;
    k->k_IOParam.m_Base = (char *)&k->dent;
    /* ��Ŀ¼��д�븸Ŀ¼�ļ� */
    k->pdir->WriteI();
    k->gInodeTable()->IPut(k->pdir);
    return pInode;
}

void FileManager::Close()
{
    _Kernel* k = _Kernel::getInstance();
    File* pFile = k->gOpenFiles()->GetF(k->fd);
    if (pFile == NULL)
        return;
    k->gOpenFiles()->SetF(k->fd, NULL);
    /* ������ϵͳ���ļ�����File�����ü��� */
    k->gOpenFileTable()->CloseF(pFile);
}


void FileManager::ChDir()
{
    Inode* pInode;
    _Kernel* k = _Kernel::getInstance();

    pInode = this->NameI(FileManager::NextChar, FileManager::OPEN);
    if (pInode == NULL)
        return;
    /* ���������ļ������� */
    if ((pInode->i_mode & Inode::IFMT) != Inode::IFDIR)
    {
        k->error = _Kernel::NOTDIR;
        k->gInodeTable()->IPut(pInode);
        return;
    }
    k->gInodeTable()->IPut(k->cdir);
    k->cdir = pInode;

    /* ���õ�ǰ����Ŀ¼�ַ���curdir */
    if (k->pathname[0] != '/')
    {
        int length = Utility::strlen(k->curdir);
        if (k->curdir[length - 1] != '/')
        {
            k->curdir[length] = '/';
            length++;
        }
        Utility::StringCopy(k->pathname, k->curdir + length);
    }
        /* ����ǴӸ�Ŀ¼'/'��ʼ����ȡ��ԭ�й���Ŀ¼ */
    else
        Utility::StringCopy(k->pathname, k->curdir);
}


Inode* FileManager::NameI(char(*func)(), enum DirectorySearchMode mode)
{
    Inode* pInode;
    Buf* pBuf;
    char curchar;
    char* pChar;
    int freeEntryOffset;	         /* �Դ����ļ�ģʽ����Ŀ¼ʱ����¼����Ŀ¼���ƫ���� */
    _Kernel* k = _Kernel::getInstance();
    BufferManager* bufMgr = k->gBufferManager();

    pInode = k->cdir;
    if ('/' == (curchar = (*func)()))
        pInode = this->rootDirInode;
    k->gInodeTable()->IGet(pInode->i_number);

    /* �������////a//b ����·�� ����·���ȼ���/a/b */
    while ('/' == curchar)
        curchar = (*func)();

    /* �����ͼ���ĺ�ɾ����ǰĿ¼�ļ������ */
    if ('\0' == curchar && mode != FileManager::OPEN)
        goto out;

    /* ���ѭ��ÿ�δ���pathname��һ��·������ */
    while (true)
    {
        if (k->error != _Kernel::NO_ERROR)
            break;
        /* ����·��������ϣ�������ӦInodeָ�롣Ŀ¼�����ɹ����ء� */
        if ('\0' == curchar)
            return pInode;
        /* ��Pathname�е�ǰ׼������ƥ���·������������_Kernel��dbuf[]�� */
        pChar = &(k->dbuf[0]);
        while ('/' != curchar && '\0' != curchar && k->error == _Kernel::NO_ERROR)
        {
            if (pChar < &(k->dbuf[DirectoryEntry::DIRSIZ]))
            {
                *pChar = curchar;
                pChar++;
            }
            curchar = (*func)();
        }
        /* ��dbufʣ��Ĳ������Ϊ'\0' */
        while (pChar < &(k->dbuf[DirectoryEntry::DIRSIZ]))
        {
            *pChar = '\0';
            pChar++;
        }

        /* �������////a//b ����·�� ����·���ȼ���/a/b */
        while ('/' == curchar)
            curchar = (*func)();
        if (k->error != _Kernel::NO_ERROR)
            return NULL;

        /* �ڲ�ѭ�����ֶ���dbuf[]�е�·���������������Ѱƥ���Ŀ¼�� */
        k->k_IOParam.m_Offset = 0;
        /* ����ΪĿ¼����� �����հ׵�Ŀ¼��*/
        k->k_IOParam.m_Count = pInode->i_size / (DirectoryEntry::DIRSIZ + 4);
        freeEntryOffset = 0;
        pBuf = NULL;
        while (true)
        {

            /* ��Ŀ¼���Ѿ�������� */
            if (k->k_IOParam.m_Count == 0)
            {
                if (pBuf != NULL)
                    bufMgr->Brelse(pBuf);
                /* ����Ǵ������ļ� */
                if (FileManager::CREATE == mode && curchar == '\0')
                {
                    /* ����Ŀ¼Inodeָ�뱣���������Ժ�дĿ¼��WriteDir()�������õ� */
                    k->pdir = pInode;
                    if (freeEntryOffset)	/* �˱�������˿���Ŀ¼��λ��Ŀ¼�ļ��е�ƫ���� */

                        /* ������Ŀ¼��ƫ��������u���У�дĿ¼��WriteDir()���õ� */
                        k->k_IOParam.m_Offset = freeEntryOffset - (DirectoryEntry::DIRSIZ + 4);

                    else
                        pInode->i_flag |= Inode::IUPD;
                    /* �ҵ�����д��Ŀ���Ŀ¼��λ�ã�NameI()�������� */
                    return NULL;
                }
                /* Ŀ¼��������϶�û���ҵ�ƥ����ͷ����Inode��Դ */
                k->error = _Kernel::NOENT;
                goto out;
            }

            /* �Ѷ���Ŀ¼�ļ��ĵ�ǰ�̿飬��Ҫ������һĿ¼�������̿� */
            if (k->k_IOParam.m_Offset % Inode::BLOCK_SIZE == 0)
            {

                if (pBuf != NULL)
                    bufMgr->Brelse(pBuf);
                /* ����Ҫ���������̿�� */
                int phyBlkno = pInode->Bmap(k->k_IOParam.m_Offset / Inode::BLOCK_SIZE);
                pBuf = bufMgr->Bread(phyBlkno);
            }

            /* û�ж��굱ǰĿ¼���̿飬���ȡ��һĿ¼����dent */
            int* src = (int *)(pBuf->b_addr + (k->k_IOParam.m_Offset % Inode::BLOCK_SIZE));
            Utility::copy<int>(src, (int *)&k->dent, sizeof(DirectoryEntry) / sizeof(int));

            k->k_IOParam.m_Offset += (DirectoryEntry::DIRSIZ + 4);
            k->k_IOParam.m_Count--;

            /* ����ǿ���Ŀ¼���¼����λ��Ŀ¼�ļ���ƫ���� */
            if (k->dent.inode == 0)
            {
                if (freeEntryOffset == 0)
                    freeEntryOffset = k->k_IOParam.m_Offset;
                /* ��������Ŀ¼������Ƚ���һĿ¼�� */
                continue;
            }

            int i;
            for (i = 0; i < DirectoryEntry::DIRSIZ; i++)
                if (k->dbuf[i] != k->dent.name[i])
                    break;	/* ƥ����ĳһ�ַ�����������forѭ�� */

            if (i < DirectoryEntry::DIRSIZ)
                /* ����Ҫ������Ŀ¼�����ƥ����һĿ¼�� */
                continue;
            else
                /* Ŀ¼��ƥ��ɹ����ص����While(true)ѭ�� */
                break;
        }

        if (pBuf != NULL)
            bufMgr->Brelse(pBuf);

        /* �����ɾ���������򷵻ظ�Ŀ¼Inode����Ҫɾ���ļ���Inode����dent.inode�� */
        if (FileManager::DELETE == mode && '\0' == curchar)
            return pInode;

        k->gInodeTable()->IPut(pInode);
        pInode = k->gInodeTable()->IGet(k->dent.inode);

        if (pInode == NULL)
            return NULL;
    }
    out:
    k->gInodeTable()->IPut(pInode);
    return NULL;
}


char FileManager::NextChar()
{
    _Kernel* k = _Kernel::getInstance();
    return *k->dirp++;
}

void FileManager::Read()
{
    this->Rdwr(File::FREAD);
}

void FileManager::Write()
{
    this->Rdwr(File::FWRITE);
}

void FileManager::Rdwr(enum File::FileFlags mode)
{
    File* pFile;
    _Kernel* k = _Kernel::getInstance();
    pFile = k->gOpenFiles()->GetF(k->fd);
    if (pFile == NULL)
        return;

    k->k_IOParam.m_Base = (char *)k->buf;
    k->k_IOParam.m_Count = k->nbytes;
    k->k_IOParam.m_Offset = pFile->f_offset;
    if (File::FREAD == mode)
        pFile->f_inode->ReadI();
    else
        pFile->f_inode->WriteI();
    pFile->f_offset += (k->nbytes - k->k_IOParam.m_Count);
    k->callReturn = k->nbytes - k->k_IOParam.m_Count;
}

void FileManager::Seek()
{
    File* pFile;
    _Kernel* k = _Kernel::getInstance();
    int fd = k->fd;
    pFile = k->gOpenFiles()->GetF(fd);
    if (NULL == pFile)
        return;

    int offset = k->offset;

    /* ���seekģʽ��3 ~ 5֮�䣬�򳤶ȵ�λ���ֽڱ�Ϊ512�ֽ� */
    if (k->mode > 2)
    {
        offset = offset << 9;
        k->mode -= 3;
    }

    switch (k->mode)
    {
        /* ��дλ������Ϊoffset */
        case 0:
            pFile->f_offset = offset;
            break;
            /* ��дλ�ü�offset(�����ɸ�) */
        case 1:
            pFile->f_offset += offset;
            break;
            /* ��дλ�õ���Ϊ�ļ����ȼ�offset */
        case 2:
            pFile->f_offset = pFile->f_inode->i_size + offset;
            break;
    }
}

void FileManager::Delete()
{
    Inode* pInode;
    Inode* pDeleteInode;
    _Kernel* k = _Kernel::getInstance();
    pDeleteInode = this->NameI(FileManager::NextChar, FileManager::DELETE);
    if (pDeleteInode == NULL)
        return;
    pInode = k->gInodeTable()->IGet(k->dent.inode);
    k->k_IOParam.m_Offset -= (DirectoryEntry::DIRSIZ + 4);
    k->k_IOParam.m_Base = (char *)&k->dent;
    k->k_IOParam.m_Count = DirectoryEntry::DIRSIZ + 4;
    k->dent.inode = 0;
    pDeleteInode->WriteI();
    pInode->i_nlink--;
    pInode->i_flag |= Inode::IUPD;
    k->gInodeTable()->IPut(pDeleteInode);
    k->gInodeTable()->IPut(pInode);
}