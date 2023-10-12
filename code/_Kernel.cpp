#include "_Kernel.h"
#include "Utility.h"
#include <fstream>

_Kernel _Kernel::_kernel_instance;

_Kernel::_Kernel()
{
    _Kernel::DISK_IMG = "myDisk.img";
    this->_k_buffermanager = new BufferManager;
    this->_k_fileSys = new FileSystem;
    this->_k_filemanager = new FileManager;
    this->_k_inodetable = new InodeTable;
    this->s_openFiles = new OpenFileTable;
    this->_k_openfiles = new OpenFiles;
    this->_k_superblock = new SuperBlock;
}

_Kernel::~_Kernel()
{
}

void _Kernel::clear()
{
    delete this->_k_buffermanager;
    delete this->_k_fileSys;
    delete this->_k_filemanager;
    delete this->_k_inodetable;
    delete this->s_openFiles;
    delete this->_k_openfiles;
    delete this->_k_superblock;
}

void _Kernel::initialize()
{
    this->_k_fileSys->LoadSuperBlock();
    this->_k_filemanager->rootDirInode = this->_k_inodetable->IGet(FileSystem::ROOTINO);
    this->cdir = this->_k_filemanager->rootDirInode;
    Utility::StringCopy("/", this->curdir);
}

void _Kernel::callInit()
{
    this->_k_filemanager->rootDirInode = this->_k_inodetable->IGet(FileSystem::ROOTINO);
    this->callReturn = -1;
    this->error = NO_ERROR;
}

_Kernel* _Kernel::getInstance()
{
    return &_kernel_instance;
}

BufferManager* _Kernel::gBufferManager()
{
    return this->_k_buffermanager;
}

FileSystem* _Kernel::gFileSystem()
{
    return this->_k_fileSys;
}

FileManager* _Kernel::gFileManager()
{
    return this->_k_filemanager;
}

InodeTable* _Kernel::gInodeTable()
{
    return this->_k_inodetable;
}

OpenFiles* _Kernel::gOpenFiles()
{
    return this->_k_openfiles;
}

OpenFileTable* _Kernel::gOpenFileTable()
{
    return this->s_openFiles;
}

SuperBlock* _Kernel::gSuperBlock()
{
    return this->_k_superblock;
}


void _Kernel::fformat()
{
    /* 格式化磁盘 */
    fstream f(_Kernel::DISK_IMG, ios::out | ios::binary);
    for (int i = 0; i <= this->gFileSystem()->DATA_ZONE_END_SECTOR; i++)
        for (int j = 0; j < this->gBufferManager()->BUFFER_SIZE; j++)
            f.write((char*)" ", 1);
    f.close();
    /* 格式化SuperBlock */
    Buf* pBuf;
    SuperBlock &spb = (*this->_k_superblock);
    spb.s_isize = FileSystem::INODE_ZONE_SIZE;
    spb.s_fsize = FileSystem::DATA_ZONE_SIZE;
    spb.s_ninode = 100;
    spb.s_nfree = 0;

    for (int i = 0; i < 100; i++)
    {
        spb.s_inode[99 - i] = i + 1;
    }

    for (int i = FileSystem::DATA_ZONE_END_SECTOR; i >= FileSystem::DATA_ZONE_START_SECTOR; i--)
    {
        this->_k_fileSys->Free(i);
    }

    for (int i = 0; i < 2; i++)
    {
        int* p = (int *)&spb + i * 128;
        pBuf = this->_k_buffermanager->GetBlk(FileSystem::SUPER_BLOCK_SECTOR_NUMBER + i);
        Utility::copy<int>(p, (int *)pBuf->b_addr, 128);
        this->_k_buffermanager->Bwrite(pBuf);
    }

    /* 格式化Inode区 */
    for (int i = 0; i < FileSystem::INODE_ZONE_SIZE; i++)
    {
        pBuf = this->_k_buffermanager->GetBlk(FileSystem::ROOTINO + FileSystem::INODE_ZONE_START_SECTOR + i);
        DiskInode DiskInode[FileSystem::INODE_NUMBER_PER_SECTOR];
        for (int j = 0; j < FileSystem::INODE_NUMBER_PER_SECTOR; j++)
        {
            DiskInode[j].d_mode = DiskInode[j].d_nlink = DiskInode[j].d_size = 0;
            for (int k = 0; k < 10; k++)
                DiskInode[j].d_addr[k] = 0;
        }
        /* 为根目录增加目录标志 */
        if (i == 0)
            DiskInode[0].d_mode |= Inode::IFDIR;
        Utility::copy<int>((int*)&DiskInode, (int*)pBuf->b_addr, 128);
        this->_k_buffermanager->Bwrite(pBuf);
    }
    this->clear();
    this->_k_buffermanager = new BufferManager;
    this->_k_fileSys = new FileSystem;
    this->_k_filemanager = new FileManager;
    this->_k_inodetable = new InodeTable;
    this->s_openFiles = new OpenFileTable;
    this->_k_openfiles = new OpenFiles;
    this->_k_superblock = new SuperBlock;
}

int _Kernel::fopen(char* pathname, int mode)
{
    this->callInit();
    this->mode = mode;
    this->pathname = this->dirp = pathname;
    this->_k_filemanager->Open();
    return this->callReturn;
}

int _Kernel::fcreat(char* pathname, int mode)
{
    this->callInit();
    this->isDir = false;
    this->mode = mode;
    this->pathname = this->dirp = pathname;
    this->_k_filemanager->Creat();
    this->_k_fileSys->Update();
    return this->callReturn;
}

void _Kernel::mkdir(char* pathname)
{
    this->callInit();
    this->isDir = true;
    this->pathname = this->dirp = pathname;
    this->_k_filemanager->Creat();
    this->_k_fileSys->Update();
    if (this->callReturn != -1)
        this->fclose(this->callReturn);
}

int _Kernel::fclose(int fd)
{
    this->callInit();
    this->fd = fd;
    this->_k_filemanager->Close();
    //this->fileSys->Update();
    return this->callReturn;
}

void _Kernel::cd(char* pathname)
{
    this->callInit();
    this->pathname = this->dirp = pathname;
    this->_k_filemanager->ChDir();
}

int _Kernel::fread(int readFd, char* buf, int nbytes)
{
    this->callInit();
    this->fd = readFd;
    this->buf = buf;
    this->nbytes = nbytes;
    this->_k_filemanager->Read();
    return this->callReturn;
}

int _Kernel::fwrite(int writeFd, char* buf, int nbytes)
{
    this->callInit();
    this->fd = writeFd;
    this->buf = buf;
    this->nbytes = nbytes;
    this->gFileManager()->Write();
    return this->callReturn;
}

void _Kernel::ls()
{
    this->k_IOParam.m_Offset = 0;
    this->k_IOParam.m_Count = this->cdir->i_size / (DirectoryEntry::DIRSIZ + 4);
    Buf* pBuf = NULL;
    while (true)
    {
        /* 对目录项已经搜索完毕 */
        if (this->k_IOParam.m_Count == 0)
        {
            if (pBuf != NULL)
                this->gBufferManager()->Brelse(pBuf);
            break;
        }

        /* 已读完目录文件的当前盘块，需要读入下一目录项数据盘块 */
        if (this->k_IOParam.m_Offset % Inode::BLOCK_SIZE == 0)
        {
            if (pBuf != NULL)
                this->gBufferManager()->Brelse(pBuf);
            int phyBlkno = this->cdir->Bmap(this->k_IOParam.m_Offset / Inode::BLOCK_SIZE);
            pBuf = this->gBufferManager()->Bread(phyBlkno);
        }

        /* 没有读完当前目录项盘块，则读取下一目录项至dent */
        int* src = (int *)(pBuf->b_addr + (this->k_IOParam.m_Offset % Inode::BLOCK_SIZE));
        Utility::copy<int>(src, (int *)&this->dent, sizeof(DirectoryEntry) / sizeof(int));
        this->k_IOParam.m_Offset += (DirectoryEntry::DIRSIZ + 4);
        this->k_IOParam.m_Count--;
        if (this->dent.inode == 0)
            continue;
        cout << this->dent.name << " ";
    }
    cout << endl;
}

void _Kernel::fseek(int seekFd, int offset, int ptrname)
{
    this->callInit();
    this->fd = seekFd;
    this->offset = offset;
    this->mode = ptrname;
    this->_k_filemanager->Seek();
}

void _Kernel::fdelete(char* pathname)
{
    this->callInit();
    this->dirp = pathname;
    this->_k_filemanager->Delete();
}

void _Kernel::fmount(char* from, char* to)
{
    fstream f(from, ios::in | ios::binary);
    if (f)
    {
        f.seekg(0, f.end);
        int length = f.tellg();
        f.seekg(0, f.beg);
        char* tmpBuffer = new char[length];
        f.read(tmpBuffer, length);
        int tmpFd = this->fopen(to, 511);
        if (this->error != NO_ERROR)
            goto end;
        this->fwrite(tmpFd, tmpBuffer, length);
        if (this->error != NO_ERROR)
            goto end;
        this->fclose(tmpFd);
        end:
        f.close();
        delete tmpBuffer;
        return;
    }
    else
    {
        this->error = NOOUTENT;
        return;
    }
}
