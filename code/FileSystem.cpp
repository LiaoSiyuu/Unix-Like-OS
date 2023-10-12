#include "FileSystem.h"
#include "Utility.h"
#include "_Kernel.h"

FileSystem::FileSystem()
{

}

FileSystem::~FileSystem()
{

}

void FileSystem::LoadSuperBlock()
{
    _Kernel* k = _Kernel::getInstance();
    BufferManager* bufMgr = k->gBufferManager();
    SuperBlock* spb = k->gSuperBlock();
    Buf* pBuf;

    for (int i = 0; i < 2; i++)
    {
        int* p = (int *)spb + i * 128;
        pBuf = bufMgr->Bread(FileSystem::SUPER_BLOCK_SECTOR_NUMBER + i);
        Utility::copy<int>((int *)pBuf->b_addr, p, 128);
        bufMgr->Brelse(pBuf);
    }
}

void FileSystem::Update()
{
    SuperBlock* spb = _Kernel::getInstance()->gSuperBlock();
    BufferManager* bufMgr = _Kernel::getInstance()->gBufferManager();
    Buf* pBuf;

    /* ͬ��SuperBlock������ */
    if (spb->s_fmod == 0)
        return;

    /* ��SuperBlock�޸ı�־ */
    spb->s_fmod = 0;

    for (int i = 0; i < 2; i++)
    {
        int* p = (int *)spb + i * 128;
        pBuf = bufMgr->GetBlk(FileSystem::SUPER_BLOCK_SECTOR_NUMBER + i);
        Utility::copy<int>(p, (int *)pBuf->b_addr, 128);
        bufMgr->Bwrite(pBuf);
    }

    /* ͬ���޸Ĺ����ڴ�Inode����Ӧ���Inode */
    InodeTable* k_InodeTable = _Kernel::getInstance()->gInodeTable();
    k_InodeTable->UpdateInodeTable();

    /* ���ӳ�д�Ļ����д�������� */
    bufMgr->Bflush();
}

Buf* FileSystem::Alloc()
{
    int blkno;	/* ���䵽�Ŀ��д��̿��� */
    SuperBlock* spb = _Kernel::getInstance()->gSuperBlock();
    BufferManager* bufMgr = _Kernel::getInstance()->gBufferManager();
    Buf* pBuf;
    _Kernel* k = _Kernel::getInstance();
    blkno = spb->s_free[--spb->s_nfree];

    if (blkno == 0)
    {
        k->error = _Kernel::NOSPACE;
        spb->s_nfree = 0;
        return NULL;
    }

    if (spb->s_nfree <= 0)
    {

        /* ����ÿ��д��̿� */
        pBuf = bufMgr->Bread(blkno);

        /* �Ӹô��̿��0�ֽڿ�ʼ��¼����ռ��4(s_nfree)+400(s_free[100])���ֽ� */
        int* p = (int *)pBuf->b_addr;

        /* ���ȶ��������̿���s_nfree */
        spb->s_nfree = *p++;

        /* ��ȡ�����к���λ�õ����ݣ�д�뵽SuperBlock�����̿�������s_free[100]�� */
        Utility::copy<int>(p, spb->s_free, 100);

        bufMgr->Brelse(pBuf);
    }

    pBuf = bufMgr->GetBlk(blkno);	        /* Ϊ�ô��̿����뻺�� */
    bufMgr->ClrBuf(pBuf);	                /* ��ջ����е����� */
    spb->s_fmod = 1;	                    /* ����SuperBlock���޸ı�־ */

    return pBuf;
}

Inode* FileSystem::IAlloc()
{
    SuperBlock* spb = _Kernel::getInstance()->gSuperBlock();
    BufferManager* bufMgr = _Kernel::getInstance()->gBufferManager();
    Buf* pBuf;
    Inode* pNode;
    _Kernel* k = _Kernel::getInstance();
    InodeTable* k_InodeTable = _Kernel::getInstance()->gInodeTable();

    int ino;	/* ���䵽�Ŀ������Inode��� */

    /* SuperBlockֱ�ӹ���Ŀ���Inode�������ѿգ�����������������Inode��*/
    if (spb->s_ninode <= 0)
    {
        ino = -1;

        /* ���ζ������Inode���еĴ��̿飬�������п������Inode���������Inode������ */
        for (int i = 0; i < spb->s_isize; i++)
        {
            pBuf = bufMgr->Bread(FileSystem::INODE_ZONE_START_SECTOR + i);

            /* ��ȡ��������ַ */
            int* p = (int *)pBuf->b_addr;

            /* ���û�������ÿ�����Inode����i_mode != 0����ʾ�Ѿ���ռ�� */
            for (int j = 0; j < FileSystem::INODE_NUMBER_PER_SECTOR; j++)
            {
                ino++;
                int mode = *(p + j * sizeof(DiskInode) / sizeof(int));

                /* �����Inode�ѱ�ռ�ã����ܼ������Inode������ */
                if (mode != 0)
                {
                    continue;
                }

                /*
                 * ������inode��i_mode==0����ʱ������ȷ��
                 * ��inode�ǿ��еģ���Ϊ�п������ڴ�inodeû��д��
                 * ������,����Ҫ���������ڴ�inode���Ƿ�����Ӧ����
                 */
                if (k_InodeTable->IsLoaded(ino) == -1)
                {
                    spb->s_inode[spb->s_ninode++] = ino;
                    if (spb->s_ninode >= 100)
                        break;
                }
            }

            /* �����Ѷ��굱ǰ���̿飬�ͷ���Ӧ�Ļ��� */
            bufMgr->Brelse(pBuf);
            if (spb->s_ninode >= 100)
                break;
        }

        if (spb->s_ninode <= 0)
            return NULL;
    }

    while (true)
    {
        ino = spb->s_inode[--spb->s_ninode];
        pNode = k_InodeTable->IGet(ino);

        if (pNode == NULL)
            return NULL;

        /* �����Inode����,���Inode�е����� */
        if (pNode->i_mode == 0)
        {
            pNode->Clean();
            /* ����SuperBlock���޸ı�־ */
            spb->s_fmod = 1;
            return pNode;
        }
        else	/* �����Inode�ѱ�ռ�� */
        {
            k_InodeTable->IPut(pNode);
            continue;	/* whileѭ�� */
        }
    }
    return NULL;
}

void FileSystem::IFree(int number)
{
    SuperBlock* spb = _Kernel::getInstance()->gSuperBlock();
    if (spb->s_ninode >= 100)
        return;
    spb->s_inode[spb->s_ninode++] = number;
    spb->s_fmod = 1;
}

void FileSystem::Free(int blkno)
{
    SuperBlock* spb = _Kernel::getInstance()->gSuperBlock();
    Buf* pBuf;
    BufferManager* bufMgr = _Kernel::getInstance()->gBufferManager();

    spb->s_fmod = 1;
    if (spb->s_nfree <= 0)
    {
        spb->s_nfree = 1;
        spb->s_free[0] = 0;
    }

    if (spb->s_nfree >= 100)
    {
        pBuf =  bufMgr->GetBlk(blkno);
        /* �Ӹô��̿��0�ֽڿ�ʼ��¼����ռ��4(s_nfree)+400(s_free[100])���ֽ� */
        int* p = (int *)pBuf->b_addr;
        /* ����д������̿��������˵�һ��Ϊ99�飬����ÿ�鶼��100�� */
        *p++ = spb->s_nfree;
        /* ��SuperBlock�Ŀ����̿�������s_free[100]д�뻺���к���λ�� */
        Utility::copy<int>(spb->s_free, p, 100);
        spb->s_nfree = 0;
        /* ����ſ����̿�������ġ���ǰ�ͷ��̿顱д����̣���ʵ���˿����̿��¼�����̿�ŵ�Ŀ�� */
        bufMgr->Bwrite(pBuf);
    }
    spb->s_free[spb->s_nfree++] = blkno;	/* SuperBlock�м�¼�µ�ǰ�ͷ��̿�� */
    spb->s_fmod = 1;
}