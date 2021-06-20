#include "FileMemoryManager.h"

/* ö���ࣺ ������ʶ���� */
enum Buffflag {
	B_DONE,
	B_WRITE,
	B_READ,
	B_DELWRI
};

/*
	�������������
	��Ա�������ͣ�
		b_flags: �����ı�ʶ
		fileno: �ļ���Ӧ��inode��
		blkno: ������Ӧ�Ĵ����߼����
		w_count: �������д��ڵ���Ч�ֽ���
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
	��������������������
	��Ա�������ͣ�
		freelist: ���ɶ���
		m_buf: ������ƿ�����
		buffer: ����������
	���ܽ��ͣ������������ٻ���ṹ
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
		/* ��ʼʱ*/
		memset(buffer, 0, sizeof(buffer));
	}

	/*
		��������: ����������ļ�Inode���ļ��߼���ţ�����һ��������
		�������ͣ�
			fino: �ļ�Inode���
			blk: �ļ����߼����
			inodetable: �ڴ������ڵ��
		����ֵ���ͣ�����Ļ�������
	*/
	int GetBlk(const int fino, const int blk, InodeTable *inodetable)
	{
		/* �����ɶ�����Ѱ���ļ���ź��߼���ž���Ӧ�Ļ������*/
		for (auto iter = freelist.begin(); iter != freelist.end(); iter++) {
			if (m_buf[*iter].get_blkno() == blk && m_buf[*iter].get_fileno() == fino) {
				int flag = *iter;
				freelist.erase(iter);
				return flag;
			}
		}
		/* ȡ�����ɶ��еĵ�һ��������ƿ� */
		while (1) {
			int first_freelist = freelist.front();
			freelist.pop_front();

			/* ����ÿ��ƿ����ӳ�д��ʶ�������Bwrite����д��*/
			if (m_buf[first_freelist].get_flag() == B_DELWRI) {
				Bwrite(first_freelist, inodetable);
			}
			else {
				m_buf[first_freelist].set_blkno(blk);
				m_buf[first_freelist].set_fileno(fino);
				m_buf[first_freelist].set_flag(B_DONE);
				/* ����Ӧ�����̿����Ϣ���뻺��� */
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
		��������: ��������Ļ������ţ��ͷŻ����
		�������ͣ�
			buffer_index: ��������
	*/
	void BuffRelease(const int buffer_index)
	{
		freelist.push_back(buffer_index);
		m_buf[buffer_index].set_flag(B_DONE);
	}

	/*
		��������: ��������Ļ������ţ���ȡ�����
		�������ͣ�
			buffer_index: ��������
	*/
	Buf *GetBufferByIndex(const int buffer_index)
	{
		return &m_buf[buffer_index];
	}

	/*
		��������: ��������е�����д��DiskInode
		�������ͣ�
			buffer_index: ��������
			inodetable: �ڴ�Inode�ڵ��
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
		��������: ɾ���ļ�ʱ���ø��ļ�Inode�ڵ�����Ӧ�����
		�������ͣ�
			inode_index: �ļ�inode�ڵ�
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
		��������: �ر��ļ�ʱ��ʹ��DELWRI��ʶ�Ļ��������д��
		�������ͣ�
			inode_index: �ļ�inode�ڵ�
			inodetable: �ڴ�Inode�ڵ��
	*/
	void WriteRelseBuffer(const int inode_index, InodeTable *inodetable)
	{
		for (int i = 0; i < sizeof(m_buf) / sizeof(Buf); i++) {
			if (m_buf[i].get_fileno() == inodetable->get_Inode(inode_index)->get_number() && m_buf[i].get_flag() == B_DELWRI) {
				//�����ɶ�����ȡ�� Ȼ���ͷ�
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
		��������: ��ȡ��Ӧ�������е�����
		�������ͣ�
			buffer_index: ���������к�
		����ֵ����: �������ַ��������ַ
	*/
	char* GetBufferComment(const int buffer_index)
	{
		return buffer[buffer_index];
	}

	/*
		��������: ��������ͷŵ����ɶ��ж�β
		�������ͣ�
			buffer_index: ���������к�
	*/
	void FreelistRelse(const int buffer_index)
	{
		freelist.push_back(buffer_index);
	}
};