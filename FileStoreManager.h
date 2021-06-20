#define _CRT_SECURE_NO_WARNINGS
#include "StoreSegment.h"

const int OFFSET = 200;
/*
	���������ļ��洢������
	��Ա�������ͣ�
		superblock: �ļ��������ʵ����
		diskinode: ���inode��ʵ����
		datadisk: ���������̿��ʵ����
*/
class FileStoreManage {
	SuperBlock superblock;
	DiskInode diskinode[6576];

public:
	/*
		��������: ���ļ��ж�ȡ��ǰ���ڵ�SuperBlock��Inode���
	*/
	FileStoreManage()
	{
		superblock.Load();
		for (int i = 0; i < sizeof(diskinode) / sizeof(DiskInode); i++)
			diskinode[i].Load(i);
	}

	/*
		��������: ����ļ��洢����ĸ�ʽ������
		���ܽ��ͣ��γ�superblock����������
	*/
	void format()
	{
		memset(this, 0, sizeof(FileStoreManage));
		for (int i = 0; i < sizeof(diskinode) / sizeof(DiskInode); i++)
			FileStoreInodeFree(i);
		for (int i = 0; i < sizeof(datadisk) / sizeof(DataDisk); i++)
			FileStoreDiskFree(i);
	}

	/*
		��������: ��superblock��s_inode�����з���һ�����е������ڵ㲢����
		����ֵ���ͣ�����Ŀ�����������ڵ�
	*/
	int FileStoreInodeAlloc()
	{
		bool need_fill;
		int tmp = superblock.inode_alloc(need_fill);
		diskinode[tmp].set_dmode_alloc(true);
		if (need_fill) {
			int cnt = 0;
			for (int i = 0; i < sizeof(diskinode) / sizeof(DiskInode); i++) {
				bool is_alloc;
				diskinode[i].get_dmode_alloc(&is_alloc);
				if (!is_alloc) {
					superblock.inode_free(i);
					cnt++;
				}
				if (cnt == max_node)
					break;
			}
		}
		/* ��������ɺ�s_ninode����0�����ʾ���п��нڵ��Ѿ�������� */
		//assert(superblock.get_sninode() > 0);

		memset(&diskinode[tmp], 0, sizeof(DiskInode));
		return tmp;
	}

	/*
		��������: ����һ�������ڵ㲢�������s_inode������
		�������ͣ�
			index: ������յ���������ڵ��±�
	*/
	void FileStoreInodeFree(const int index)
	{
		diskinode[index].set_dmode_alloc(false);
		superblock.inode_free(index);
	}

	/*
		��������: ��superblock��s_free�����з���һ�����е������̿鲢����
		����ֵ���ͣ�����Ŀ��������̿�
	*/
	int FileStoreDiskAlloc()
	{
		bool need_fill;
		int tmp = superblock.disk_alloc(need_fill);
		if (need_fill) {
			int buffer = 0;
			/* ʹ��s_free[0]ȥ�ж���һ�����������Ƿ���� */
			memcpy(&buffer, tmp, sizeof(int), sizeof(int));
			/* ����s_free[0] Ϊ0�� �����п������ݿ��Ѿ�������� */
			assert(buffer != 0);

			/* ����һ���������ӵ�ֵ�������s_free��s_nfree*/
			buffer = 0;
			memcpy(&buffer, tmp, 0, sizeof(int));
			superblock.set_snfree(buffer);

			for (int i = 0; i < superblock.get_snfree(); i++) {
				buffer = 0;
				memcpy(&buffer, tmp, (i + 1) * sizeof(int), sizeof(int));
				superblock.set_sfree(i, buffer);
			}
		}
		char buf[512];
		memset(buf, 0, sizeof(buf));
		memcpy(tmp, buf, 0, BLOCK_SIZE);
		return tmp;
	}

	/*
		��������: ����һ�����������̿鲢�������s_free������
		�������ͣ�
			index: ������յĿ��������̿��±�
	*/
	void FileStoreDiskFree(const int index)
	{
		bool need_fill;
		superblock.disk_free(index, need_fill);
		if (need_fill) {
			/* ��index�����̿��ǰ404�ֽ����뵱ǰsuperblock��s_nfree �� s_free*/
			int buffer = superblock.get_snfree();
			memcpy(index, &buffer, 0, sizeof(int));

			for (int i = 0; i < 100; i++) {
				buffer = superblock.get_sfree(i);
				memcpy(index, &buffer, (i + 1) * sizeof(int), sizeof(int));
			}

			/* ����s_nfree �� s_free*/
			superblock.set_snfree(1);
			superblock.set_sfree(0, index);
		}
	}

	/*
		��������: ����һ��diskinode�ڵ㣬���ڴ�inode���ṩ�ӿ�
		�������ͣ�
			i_number: �����ȡ�ĵ�i_number��diskinode�ڵ�
	*/
	DiskInode * FileStoreGetDiskInode(const int i_number)
	{
		return &diskinode[i_number];
	}

	/*
		��������: ����һ��diskinode�ڵ㣬���ڴ�inode���ṩ�ӿ�
		�������ͣ�
			inode: ��inode����diskinode�ڵ�
	*/
	void FileStoreSetDiskInode(Inode &inode)
	{
		int i_number = inode.get_number();
		diskinode[i_number].set_dnlink(inode.get_nlink());
		diskinode[i_number].set_dsize(inode.get_size());
		for (int i = 0; i < 10; i++)
			diskinode[i_number][i] = inode[i];
		if (inode[6])
			diskinode[i_number].set_dmode_file_size(true);
	}

	/*
	��������: ������Ŀ¼�ļ���diskinode�еǼ���ͨ�ļ���Ŀ¼�ļ�
	�������ͣ�
		dir_index: Ҫ�Ǽǵ�Ŀ¼�ļ�����Ӧ��diskinode���
		reg_name: ����Ǽǵ��ļ���
		diskinode_index: ����Ǽǵ��ļ�����Ӧ��diskInode�����
	*/
	void file_reg(const int dir_index, const string reg_name, const int diskinode_index)
	{
		for (int i = 0; i <= 5; i++) {
			if (diskinode[dir_index][i] != 0 && datadisk[diskinode[dir_index][i] - 1].file_register(reg_name, diskinode_index, diskinode[dir_index][i] - 1) == true)
				return;
			else if (diskinode[dir_index][i] == 0) {
				/* ��Ҫ����һ���µĿ����̿飬���ڵǼ���Ϣ */
				int free_block = FileStoreDiskAlloc();
				/* ������õ����ݿ�����+1���Ǽǵ�d_addr������ */
				diskinode[dir_index][i] = free_block + 1;
				datadisk[diskinode[dir_index][i] - 1].file_register(reg_name, diskinode_index, diskinode[dir_index][i] - 1);
				return;
			}
		}
		for (int i = 6; i <= 7; i++) {
			if (diskinode[dir_index][i] != 0) {
				int buffer = diskinode[dir_index][i] - 1;
				for (int j = 0; j < BLOCK_SIZE / sizeof(int); j++) {
					int block_index = 0;
					memcpy(&block_index, buffer, j * sizeof(int), sizeof(int));
					if (block_index != 0 && datadisk[block_index - 1].file_register(reg_name, diskinode_index, block_index - 1) == true)
						return;
					else if (block_index == 0) {
						int free_block = FileStoreDiskAlloc();
						free_block += 1;
						memcpy(buffer, &free_block, j * sizeof(int), sizeof(int));
						datadisk[free_block - 1].file_register(reg_name, diskinode_index, free_block - 1);
						return;
					}
				}
			}
			else {
				int free_block = FileStoreDiskAlloc();
				diskinode[dir_index][i] = free_block + 1;
				/* �ٴ�ִ��ѭ�� */
				i--;
			}
		}
		for (int i = 8; i <= 9; i++) {
			if (diskinode[dir_index][i] != 0) {
				int buffer1 = diskinode[dir_index][i] - 1;
				for (int j = 0; j < BLOCK_SIZE / sizeof(int); j++) {
					int buffer2 = 0;
					memcpy(&buffer2, buffer1, j * sizeof(int), sizeof(int));
					if (buffer2 != 0) {
						buffer2 -= 1;
						for (int k = 0; k < BLOCK_SIZE / sizeof(int); k++) {
							int block_index = 0;
							memcpy(&block_index, buffer2, k * sizeof(int), sizeof(int));
							if (block_index != 0 && datadisk[block_index - 1].file_register(reg_name, diskinode_index, block_index - 1) == true)
								return;
							else if (block_index == 0) {
								int free_block = FileStoreDiskAlloc();
								free_block += 1;
								// ��ֵӦ����k
								memcpy(buffer2, &free_block, k * sizeof(int), sizeof(int));
								datadisk[free_block - 1].file_register(reg_name, diskinode_index, free_block - 1);
								return;
							}
						}
					}
					else {
						int free_block = FileStoreDiskAlloc();
						free_block += 1;
						memcpy(buffer1, &free_block, j * sizeof(int), sizeof(int));
						/* �ٴ�ִ��ѭ�� */
						j--;
					}
				}
			}
			else {
				int free_block = FileStoreDiskAlloc();
				diskinode[dir_index][i] = free_block + 1;
				/* �ٴ�ִ��ѭ�� */
				i--;
			}
		}
		return;
	}

	/*
	��������: ���ڽ�ĳ�����Inode�����п������ݿ���� �����ͷ������������Inode�ڵ�
	�������ͣ�
		diskindex����Ҫ���տ������ݿ�����inode���
	*/
	void FileStoreReleaseDiskInodeArray(const int diskindex)
	{
		DiskInode *tmp = FileStoreGetDiskInode(diskindex);
		FileStoreInodeFree(diskindex);
		/* С���ļ����ݿ��ͷ� */
		for (int i = 0; i <= 5; i++) {
			/* �Ǽ������е����ݿ�ʱ���������+1�ķ�ʽ���� */
			if ((*tmp)[i] != 0) {
				FileStoreDiskFree((*tmp)[i] - 1);
				(*tmp)[i] = 0;
			}
			else
				break;
		}
		/* �����ļ����ݿ��ͷ�*/
		for (int i = 6; i <= 7; i++) {
			if ((*tmp)[i] != 0) {
				int buffer = (*tmp)[i] - 1;
				for (int j = 0; j < BLOCK_SIZE / sizeof(int); j++) {
					int block_index = 0;
					memcpy(&block_index, buffer, j * sizeof(int), sizeof(int));
					if (block_index != 0)
						FileStoreDiskFree(block_index - 1);
					else
						break;
				}
				/* �ͷ������̿�� ��һ�������̿�ͬ���ͷ�*/
				FileStoreDiskFree(buffer);
				(*tmp)[i] = 0;
			}
			else
				break;
		}

		/* �����ļ����ݿ��ͷ� */
		for (int i = 8; i <= 9; i++) {
			if ((*tmp)[i] != 0) {
				int buffer1 = (*tmp)[i] - 1;
				for (int j = 0; j < BLOCK_SIZE / sizeof(int); j++) {
					int buffer2 = 0;
					memcpy(&buffer2, buffer1, j * sizeof(int), sizeof(int));
					if (buffer2 != 0) {
						buffer2 -= 1;
						for (int k = 0; k < BLOCK_SIZE / sizeof(int); k++) {
							int block_index = 0;
							memcpy(&block_index, buffer2, k * sizeof(int), sizeof(int));
							if (block_index != 0)
								FileStoreDiskFree(block_index - 1);
							else
								break;
						}
						FileStoreDiskFree(buffer2);
					}
					else
						break;
				}
				FileStoreDiskFree(buffer1);
				(*tmp)[i] = 0;
			}
			else
				break;
		}
	}

	/*
	��������: ���ڽ�ĳ��ΪĿ¼�ṹ��DiskInode���洢����Ϣ�У�ɾ������һ��Ŀ¼��
	�������ͣ�
		src_index: Ŀ¼�ļ�����Ӧ��DiskInode
		diskindex: ��Ҫɾ����Ŀ¼���Ӧ������diskindex
	���ܽ��ͣ�Ѱ�ҵ�diskindex����Ӧ��Ŀ¼������һ��Ŀ¼����߽����󣬽����һ�����㼴��
	*/
	void FileStoreClearDirectoryEntry(const int src_index, const int diskindex)
	{
		/* ���Ȼ�ȡ���һ��Ŀ¼�� */
		int last_node;
		string last_name;
		diskinode[src_index].get_last_dir_entry(last_node, last_name);

		/* һ��Ҫ��ʹ�����һ����¼���Ǻ� ��ɾ�����һ����¼ */
		diskinode[src_index].disk_change_dir_entry(last_node, last_name, diskindex);
		diskinode[src_index].disk_change_dir_entry(0, "", last_node);
	}

	void save()
	{
		superblock.save();
		for (int i = 0; i < sizeof(diskinode) / sizeof(DiskInode); i++)
			diskinode[i].Save(i);
	}
};