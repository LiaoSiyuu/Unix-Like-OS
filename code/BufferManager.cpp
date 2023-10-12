#include "Buf.h"
#include "BufferManager.h"
#include "_Kernel.h"

const char* _Kernel::DISK_IMG;

BufferManager::BufferManager()
{
    Buf* bp;
    this->_bFreeList.av_forw = this->_bFreeList.av_back = &(this->_bFreeList);
    this->_bDevtab.b_forw = this->_bDevtab.b_back = &(this->_bDevtab);

    for(int i = 0; i < NBUF; i++)
    {
        bp = &(this->_m_Buf[i]);
        bp->b_addr = this->_Buffer[i];

        bp->b_back = &(this->_bDevtab);
        bp->b_forw = this->_bDevtab.b_forw;
        this->_bDevtab.b_forw->b_back = bp;
        this->_bDevtab.b_forw = bp;

        bp->b_flags = Buf::B_BUSY;
        Brelse(bp);
    }
    return;
}

BufferManager::~BufferManager()
{
    Bflush();
}

Buf* BufferManager::GetBlk(int blkno)
{
    Buf* bp;
    Buf* dp = &this->_bDevtab;

    while (true)
    {
        // 在设备队列中搜索是否有相应的缓存
        for (bp = dp->b_forw; bp != dp; bp = bp->b_forw)
        {
            if (bp->b_blkno == blkno)
            {
                if (bp->b_flags & Buf::B_BUSY)
                {
                    bp->b_flags |= Buf::B_WANTED;
                    break;
                }
                this->_NotAvail_(bp);
                return bp;
            }
        }

        // 自由队列为空，等待空闲块
        if (this->_bFreeList.av_forw == &this->_bFreeList)
        {
            this->_bFreeList.b_flags |= Buf::B_WANTED;
            continue;
        }

        // 取自由队列第一个空闲块
        bp = this->_bFreeList.av_forw;
        this->_NotAvail_(bp);

        // 如果该缓存块是延迟写，将其异步写入磁盘
        if (bp->b_flags & Buf::B_DELWRI)
        {
            bp->b_flags |= Buf::B_ASYNC;
            this->Bwrite(bp);
            continue;
        }

        bp->b_flags = Buf::B_BUSY;
        bp->b_blkno = blkno;
        return bp;
    }
}

void BufferManager::Brelse(Buf* bp)
{
    bp->b_flags &= ~(Buf::B_WANTED | Buf::B_BUSY | Buf::B_ASYNC);

    bp->av_forw = &(this->_bFreeList);
    bp->av_back = this->_bFreeList.av_back;
    (this->_bFreeList.av_back)->av_forw = bp;
    this->_bFreeList.av_back = bp;
}

void BufferManager::_NotAvail_(Buf *bp)
{
    bp->av_back->av_forw = bp->av_forw;
    bp->av_forw->av_back = bp->av_back;

    bp->b_flags |= Buf::B_BUSY;
}

Buf* BufferManager::Bread(int blkno)
{
    Buf* bp = this->GetBlk(blkno);
    if (bp->b_flags & Buf::B_DONE)
        return bp;

    bp->b_flags |= Buf::B_READ;
    bp->b_wcount = BufferManager::BUFFER_SIZE;
    ifstream f(_Kernel::DISK_IMG, ios::in | ios::binary);
    f.seekg(blkno * BufferManager::BUFFER_SIZE);
    f.read(bp->b_addr, bp->b_wcount);
    f.close();
    return bp;
}

void writing(writeArg* arg)
{
    std::fstream f(_Kernel::DISK_IMG, ios::in | ios::out | ios::binary);
    f.seekp(arg->bp->b_blkno*BufferManager::BUFFER_SIZE);
    f.write((char *)arg->bp->b_addr, arg->bp->b_wcount);
    f.close();
    arg->b->Brelse(arg->bp);
}

void BufferManager::Bwrite(Buf *bp)
{
    unsigned int flags;
    flags = bp->b_flags;
    bp->b_flags &= ~(Buf::B_READ | Buf::B_DONE | Buf::B_ERROR | Buf::B_DELWRI);
    bp->b_wcount = BufferManager::BUFFER_SIZE;		/* 512�ֽ� */
    writeArg* arg = new writeArg(this, bp);
    writing(arg);
    return;
}

void BufferManager::Bdwrite(Buf *bp)
{
    bp->b_flags |= (Buf::B_DELWRI | Buf::B_DONE);
    this->Brelse(bp);
    return;
}

void BufferManager::Bawrite(Buf *bp)
{
    bp->b_flags |= Buf::B_ASYNC;
    this->Bwrite(bp);
    return;
}

void BufferManager::Bflush()
{
    loop:
    for (Buf* bp = this->_bFreeList.av_forw; bp != &this->_bFreeList; bp = bp->av_forw)
        if (bp->b_flags & Buf::B_DELWRI)
        {

            this->_NotAvail_(bp);
            this->Bwrite(bp);
            goto loop;
        }
}


void BufferManager::ClrBuf(Buf *bp)
{
    int* pInt = (int *)bp->b_addr;
    for (int i = 0; i < BufferManager::BUFFER_SIZE / sizeof(int); i++)
        pInt[i] = 0;
}