#include "FileMemoryManager.h"

/* 枚举类： 缓冲块标识定义 */
enum Buffflag {
	B_DONE,
	B_WRITE,
	B_READ,
	B_DELWRI
};

/*
	类声明：缓冲块
	成员变量解释：
		b_flags: 缓冲块的标识
		fileno: 文件对应的inode号
		blkno: 缓冲块对应的磁盘逻辑块号
		w_count: 缓冲区中存在的有效字节数
*/
class Buf {
	unsigned int b_flag;
	int fileno;
	int blkno;
	int w_count;

public:
	Buf()
	{
		b_flag = B_DONE;
		blkno = 0;
		fileno = -1;
		w_count = 0;
	}

	int get_flag()
	{
		return b_flag;
	}

	void set_flag(const int flag)
	{
		b_flag = flag;
	}

	int get_fileno()
	{
		return fileno;
	}

	void set_fileno(const int file)
	{
		fileno = file;
	}

	int get_blkno()
	{
		return blkno;
	}

	void set_blkno(const int blk)
	{
		blkno = blk;
	}

	int get_wcount()
	{
		return w_count;
	}

	void set_wcount(const int cnt)
	{
		w_count = cnt;
	}
};

/*
	类声明：缓冲区管理类
	成员变量解释：
		freelist: 自由队列
		m_buf: 缓冲控制块数组
		buffer: 缓冲区数组
	功能解释：管理整个高速缓存结构
*/
class BufferManager {
	deque<int> freelist;
	Buf m_buf[15];
	char buffer[15][513];

public:
	BufferManager()
	{
		for (int i = 0; i < sizeof(m_buf) / sizeof(Buf); i++) {
			freelist.push_back(i);
		}
		/* 初始时*/
		memset(buffer, 0, sizeof(buffer));
	}

	/*
		函数名字: 根据输入的文件Inode和文件逻辑块号，分配一个缓冲区
		参数解释：
			fino: 文件Inode编号
			blk: 文件的逻辑块号
			inodetable: 内存索引节点表
		返回值解释：分配的缓冲块序号
	*/
	int GetBlk(const int fino, const int blk, InodeTable *inodetable)
	{
		/* 在自由队列中寻找文件编号和逻辑块号均对应的缓冲块抽出*/
		for (auto iter = freelist.begin(); iter != freelist.end(); iter++) {
			if (m_buf[*iter].get_blkno() == blk && m_buf[*iter].get_fileno() == fino) {
				int flag = *iter;
				freelist.erase(iter);
				return flag;
			}
		}
		/* 取出自由队列的第一个缓存控制块 */
		while (1) {
			int first_freelist = freelist.front();
			freelist.pop_front();

			/* 如果该控制块有延迟写标识，则调用Bwrite函数写入*/
			if (m_buf[first_freelist].get_flag() == B_DELWRI) {
				Bwrite(first_freelist, inodetable);
			}
			else {
				m_buf[first_freelist].set_blkno(blk);
				m_buf[first_freelist].set_fileno(fino);
				m_buf[first_freelist].set_flag(B_DONE);
				/* 将对应物理盘块的信息读入缓存块 */
				int inode_index;
				for (int i = 0; i < 100; i++) {
					if (inodetable->get_Inode(i)->get_number() == fino) {
						inode_index = inodetable->get_Inode(i)->get_index();
						break;
					}
				}
				memcpy(buffer[first_freelist], inodetable->Bmap(inode_index, blk) - 1, 0, BLOCK_SIZE);
				return first_freelist;
			}
		}
	}

	/*
		函数名字: 根据输入的缓冲块序号，释放缓冲块
		参数解释：
			buffer_index: 缓冲块序号
	*/
	void BuffRelease(const int buffer_index)
	{
		freelist.push_back(buffer_index);
		m_buf[buffer_index].set_flag(B_DONE);
	}

	/*
		函数名字: 根据输入的缓冲块序号，获取缓冲块
		参数解释：
			buffer_index: 缓冲块序号
	*/
	Buf *GetBufferByIndex(const int buffer_index)
	{
		return &m_buf[buffer_index];
	}

	/*
		函数名字: 将缓冲块中的内容写入DiskInode
		参数解释：
			buffer_index: 缓冲块序号
			inodetable: 内存Inode节点表
	*/
	void Bwrite(int buffer_index, InodeTable *inodetable)
	{
		DirectoryEntry tmp;
		Buf *bp = GetBufferByIndex(buffer_index);
		bp->set_wcount(BLOCK_SIZE);
		int inode_index;
		for (int i = 0; i < 100; i++) {
			if (inodetable->get_Inode(i)->get_number() == bp->get_fileno()) {
				inode_index = inodetable->get_Inode(i)->get_index();
				break;
			}
		}
		memcpy(inodetable->Bmap(inode_index, bp->get_blkno()) - 1, buffer[buffer_index], 0, BLOCK_SIZE);
		BuffRelease(buffer_index);
	}

	/*
		函数名字: 删除文件时重置该文件Inode节点所对应缓冲块
		参数解释：
			inode_index: 文件inode节点
	*/
	void UpdateRelseBuffer(const int inode_index)
	{
		for (int i = 0; i < sizeof(m_buf) / sizeof(Buf); i++) {
			if (m_buf[i].get_fileno() == inode_index) {
				m_buf[i].set_blkno(0);
				m_buf[i].set_fileno(-1);
				m_buf[i].set_flag(B_DONE);
				m_buf[i].set_wcount(0);
			}
		}
	}

	/*
		函数名字: 关闭文件时，使有DELWRI标识的缓冲块立刻写入
		参数解释：
			inode_index: 文件inode节点
			inodetable: 内存Inode节点表
	*/
	void WriteRelseBuffer(const int inode_index, InodeTable *inodetable)
	{
		for (int i = 0; i < sizeof(m_buf) / sizeof(Buf); i++) {
			if (m_buf[i].get_fileno() == inodetable->get_Inode(inode_index)->get_number() && m_buf[i].get_flag() == B_DELWRI) {
				//从自由队列中取出 然后释放
				auto p = freelist.begin();
				for (auto iter = freelist.begin(); iter != freelist.end(); iter++)
					if (*iter == i)
						p = iter;
				freelist.erase(p);
				Bwrite(i, inodetable);
			}
		}
	}

	/*
		函数名字: 获取对应缓存区中的内容
		参数解释：
			buffer_index: 缓冲块的序列号
		返回值解释: 缓冲块的字符串数组地址
	*/
	char* GetBufferComment(const int buffer_index)
	{
		return buffer[buffer_index];
	}

	/*
		函数名字: 将缓存块释放到自由队列队尾
		参数解释：
			buffer_index: 缓冲块的序列号
	*/
	void FreelistRelse(const int buffer_index)
	{
		freelist.push_back(buffer_index);
	}
};