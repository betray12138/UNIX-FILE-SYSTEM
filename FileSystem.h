#include "FileOpenManager.h"
#include <stdio.h>
/*
	���������ļ�����ϵͳ�����࣬�����ṩ���ֺ������ýӿ�
	��Ա�������ͣ�
		cur_dir_path: �洢�û���ǰ����Ŀ¼�ĸ�·��
		cur_dir_diskinode: ʹ�ÿɱ��������洢�Ӹ�·�����ﵱǰĿ¼DiskInode��Inode���  ���ں���
		root_inode_index: ���ڵ�Ŀ¼�ļ���DiskInode���
		openfiletable: ���ڹ���ϵͳ�Ĵ��ļ���
	���ܽ��ͣ������ڴ����д��ļ�
*/
class FileSys {
	string cur_dir_path;
	vector<int> cur_dir_diskinode;
	int root_inode_index;
	OpenFileTable openfiletable;

public:
	FileSys()
	{
		/* ����ʼʱ�����ڸ�Ŀ¼�� */
		cur_dir_path = "/";
		// ���ļ��ж�ȡroot_inode_index
		fp.seekg(0, ios::beg);
		fp.read((char *)&root_inode_index, sizeof(int));
		if (root_inode_index == 0) {
			// δ��ȡ��ֵ ����г�ʼ��
			/* Ϊ��Ŀ¼����һ��diskInode */
			openfiletable.get_inode_table()->get_filestoremanage()->format();
			root_inode_index = openfiletable.get_inode_table()->get_filestoremanage()->FileStoreInodeAlloc();
			DiskInode *root_inode = openfiletable.get_inode_table()->get_filestoremanage()->FileStoreGetDiskInode(root_inode_index);
			root_inode->set_dmode_file_mode(true);
			// ��root_indexд���ļ�
			fp.seekg(0, ios::beg);
			fp.write((char *)&root_inode_index, sizeof(int));
		}
		cur_dir_diskinode.push_back(root_inode_index);
	}

	/*
		��������: ���������·�� Ѱ�ҵ������һ��Ŀ¼�ṹ ���޷��ҵ�����-1 ���򷵻�
		�������ͣ�
			dir_path: ���ܾ���·�������·��������ʽ��Ҫ�����·����/��ͷ�� ���·����./��ͷ ��·��ĩβ������/
					  ·���в������������������/
			reg: �Ƿ���Ҫ�Ǽǽ���cur_dir_diskinode������
		����ֵ���ͣ������Խ������һ��Ŀ¼���򷵻�Ŀ¼��DiskInode��ţ����򷵻�-1��ʾ�޷��ҵ�
	*/
	int dir_search(string dir_path, bool reg = false)
	{
		int cur_inode_index = root_inode_index;
		/* �������·�� ����ת��Ϊ����·������ ��ͬ������cur_inode_index������Ӧ����ͬ*/
		if (dir_path[0] != '/') {
			/* ���·����ʽ ������ʼ����../��������������м����../����� */
			if (dir_path[0] != '.') {
				/* ������·������./...����ʽ�� ������ǰ�����./ */
				dir_path = "./" + dir_path;
			}
			/* ʹ��backwards_cnt ��¼���˴���*/
			int backwards_cnt = 0;
			int normal_pos = 0;
			for (int i = 0; i < dir_path.size() - 1; i++) {
				if (dir_path[i] == '.' && dir_path[i + 1] == '.') {
					backwards_cnt++;
					normal_pos = i + 2;   // normal_pos ���ڱ�����һ��../��/���ֵ�λ��
				}
				else if (dir_path[i] == '.' && dir_path[i + 1] == '/') {
					/* ���·���н���./ ���� û��../���ֵ���� */
					normal_pos = i + 1;
				}
			}
			dir_path = dir_path.substr(normal_pos, -1);
			int pos = max((int)cur_dir_diskinode.size() - 1 - backwards_cnt, 0);
			cur_inode_index = cur_dir_diskinode[pos];
			/*����Ҫ�Ǽǵ��������Ҫ��������л���, ��string���м���*/
			while (reg == true && cur_dir_diskinode[cur_dir_diskinode.size() - 1] != cur_inode_index) {
				cur_dir_diskinode.pop_back();
				int last_line_pos = cur_dir_path.rfind('/', cur_dir_path.size());
				cur_dir_path = cur_dir_path.substr(0, last_line_pos);
			}
		}
		/* �������·�� ����Ҫ�Ǽǵ���� */
		else if(reg == true){
			/* ��cur_dir_diskinode ȫ��pop ����cur_dir_path ���*/
			cur_dir_path = "/";
			cur_dir_diskinode.clear();
			cur_dir_diskinode.push_back(root_inode_index);
		}
		/* �������·�� */
		int src_pos = 1, des_pos = src_pos;
		for (int i = src_pos; i < dir_path.size(); i++) {
			if (dir_path[i] == '/') {
				des_pos = i;
				string match_dir_name = dir_path.substr(src_pos, des_pos - src_pos);
				src_pos = des_pos + 1;
				int next_layer_index = openfiletable.get_inode_table()->get_filestoremanage()->FileStoreGetDiskInode(cur_inode_index)->dir_path_match(match_dir_name);
				if (next_layer_index == -1)
					return -1;
				cur_inode_index = next_layer_index;
				/* ����Ҫ�Ǽǵ��������Ҫ��������ӣ���string��������*/
				if (reg == true) {
					cur_dir_diskinode.push_back(next_layer_index);
					cur_dir_path += string("/") + match_dir_name;
				}
			}
		}
		return cur_inode_index;
	}

	/*
		��������: �����ļ�
		�������ͣ�
			filename: �ļ�������
			is_w�� �ļ��Ķ�дȨ�� 0 -- ֻ�� 1 -- ��д
		����ֵ���ͣ�false ��ʾ����ʧ��   true ��ʾ�����ɹ�
	*/
	bool fcreate(string filename, const bool is_w)
	{
		int diskinode_index = dir_search(filename);
		if (diskinode_index == -1) {
			cout << "�����ڶ�ӦĿ¼" << endl;
			return false;
		}

		/* �����贴�����ļ�����file_name��ȡ��*/
		int line_pos = -1;
		for (int i = 0; i < filename.size(); i++)
			if (filename[i] == '/')
				line_pos = i;

		filename = filename.substr(line_pos + 1, -1);
		/* ���յ�filenameӦ�������� ab.txt */
		/* �ڵ�ǰĿ¼�¶�filename ���������� ������filename �򱨴� �������� �򴴽�filename ���ҵǼǽ�diskinode_index��*/
		int file_inode_index = openfiletable.get_inode_table()->get_filestoremanage()->FileStoreGetDiskInode(diskinode_index)->dir_path_match(filename);
		if (file_inode_index != -1) {
			/* �ļ�����*/
			cout << "��ǰĿ¼���ļ��Ѵ��ڣ��޷��ٴδ���" << endl;
			return false;
		}
		else {
			/* �ļ������� */
			/* Ϊ���ļ�����һ��DiskInode�����ҵǼǵ�Ŀ¼��diskInode�� */
			int new_diskinode_index = openfiletable.get_inode_table()->get_filestoremanage()->FileStoreInodeAlloc();
			openfiletable.get_inode_table()->get_filestoremanage()->file_reg(diskinode_index, filename, new_diskinode_index);

			DiskInode *new_disk = openfiletable.get_inode_table()->get_filestoremanage()->FileStoreGetDiskInode(new_diskinode_index);
			new_disk->set_dmode_file_mode(false);
			new_disk->set_dmode_file_size(false);
			new_disk->set_dmode_wr(is_w, true);
			new_disk->set_dnlink(1);
			new_disk->set_dsize(0);
		}
		return true;
	}

	/*
		��������: ������Ŀ¼
		�������ͣ�
			dirname����Ŀ¼������
		����ֵ���ͣ�false ��ʾ����ʧ��   true ��ʾ�����ɹ�
	*/
	bool mkdir(string dirname)
	{
		int diskinode_index = dir_search(dirname);
		if (diskinode_index == -1) {
			cout << "�����ڶ�ӦĿ¼" << endl;
			return false;
		}

		/* �����贴�����ļ�����file_name��ȡ��*/
		int line_pos = -1;
		for (int i = 0; i < dirname.size(); i++)
			if (dirname[i] == '/')
				line_pos = i;

		dirname = dirname.substr(line_pos + 1, -1);
		/* ���յ�dirnameӦ�������� root */
		/* �ڵ�ǰĿ¼�¶�dirname ���������� ������dirname �򱨴���ʾ �������� �򴴽�dirname ���ҵǼǽ�diskinode_index��*/
		int file_inode_index = openfiletable.get_inode_table()->get_filestoremanage()->FileStoreGetDiskInode(diskinode_index)->dir_path_match(dirname);
		if (file_inode_index != -1) {
			/* Ŀ¼����*/
			cout << "��ǰĿ¼���ļ��Ѵ��ڣ��޷��ٴδ���" << endl;
			return false;
		}
		else {
			/* Ŀ¼������ */
			/* Ϊ��Ŀ¼����һ��DiskInode�����ҵǼǵ�ԴĿ¼��diskInode�� */
			int new_diskinode_index = openfiletable.get_inode_table()->get_filestoremanage()->FileStoreInodeAlloc();
			openfiletable.get_inode_table()->get_filestoremanage()->file_reg(diskinode_index, dirname, new_diskinode_index);

			DiskInode *new_disk = openfiletable.get_inode_table()->get_filestoremanage()->FileStoreGetDiskInode(new_diskinode_index);
			new_disk->set_dmode_file_mode(true);
			new_disk->set_dmode_file_size(false);
			new_disk->set_dmode_wr(true, true);
		}
		return true;
	}

	/*
		��������: �鿴����·�����ļ���Ŀ¼��Ϣ
		�������ͣ�
			path_name: ��Ҫ�鿴��·������
	*/
	void ls(string path_name = "")
	{
		/* ��lsû�����������ֱ����ʾ��ǰĿ¼���ļ����� */
		int dir_path_inode_index = cur_dir_diskinode[cur_dir_diskinode.size() - 1];

		if (path_name != "") {
			path_name = (path_name[path_name.size() - 1] == '/' ? path_name : path_name + "/");
			int res = dir_search(path_name);
			if (res == -1) {
				cout << "�����·�������ڣ�����������" << endl;
				return;
			}
			else
				dir_path_inode_index = res;
		}
		DiskInode *dir_disk = openfiletable.get_inode_table()->get_filestoremanage()->FileStoreGetDiskInode(dir_path_inode_index);
		dir_disk->print_dir_item();
	}

	/*
		��������: ����ĳ��Ŀ¼ҳ����
		�������ͣ�
			path_name: ��Ҫ�����Ŀ¼·��  ����·��������ļ�  ���޷�����
	*/
	void cd(string path_name)
	{
		if (path_name[path_name.size() - 1] != '/')
			path_name += "/";
		int res = dir_search(path_name);
		if (res == -1) {
			cout << "��Ŀ¼�����ڣ� �޷������Ŀ¼" << endl;
			return;
		}
		DiskInode *disk = openfiletable.get_inode_table()->get_filestoremanage()->FileStoreGetDiskInode(res);
		bool is_dir;
		disk->get_dmode_file_mode(&is_dir);
		if (!is_dir) {
			cout << "��������Ŀ¼ʵ����һ����ͨ�ļ����޷�����" << endl;
			return;
		}
		dir_search(path_name, true);
	}

	/*
		��������: ���ļ�
		�������ͣ�
			filename: �ļ�������
			is_w�� �ļ��Ķ�дȨ�� 0 -- ֻ�� 1 -- ��д
		����ֵ���ͣ������ļ������������-1���ʧ��
	*/
	int fopen(string filename, const bool is_w)
	{
		if (filename[filename.size() - 1] != '/')
			filename += "/";
		int file_diskinode_index = dir_search(filename);
		if (file_diskinode_index == -1) {
			cout << "�ļ������ڣ���ʧ��" << endl;
			return -1;
		}
		Inode *file_inode = openfiletable.get_inode_table()->InodeGetWithIndexOfDiskInode(file_diskinode_index);
		bool wr, is_r, is_dir;
		file_inode->get_imode_dir(&is_dir);
		if (is_dir) { 
			cout << "���ļ���Ŀ¼�ļ����޷�����" << endl;
			openfiletable.get_inode_table()->InodeReduce(file_inode);
			return -1;
		}
		file_inode->get_imode_wr(&wr, &is_r);

		if (wr < is_w) {
			cout << "�ļ�û��д��Ȩ�ޣ���ʧ��" << endl;
			openfiletable.get_inode_table()->InodeReduce(file_inode);
			return -1;
		}
		/* Ϊ�ļ����ڴ�Inode����һ���µĴ��ļ��ṹ*/
		int fd;
		File * fileopen = openfiletable.FAlloc(fd);
		fileopen->set_count(1);
		fileopen->set_imode_wr(is_w, true);
		fileopen->set_offset(0);
		fileopen->set_point_inode(file_inode);
		return fd;
	}

	/*
		��������: �ر��ļ�
		�������ͣ�
			fd: �ļ����
		���ܽ��ͣ� �����ļ�����ر��ļ�
	*/
	void fclose(const int fd)
	{
		File * pfile = openfiletable.FGet(fd);
		if (pfile->get_point_inode() == nullptr) {
			cout << "����������" << endl;
			return;
		}
		openfiletable.FClose(pfile);
	}

	/*
		��������: ɾ���ļ�
		�������ͣ�
			pathname: Ҫɾ�����ļ�
	*/
	void fdelete(string pathname)
	{
		string dir_path = pathname;
		if (pathname[pathname.size() - 1] != '/')
			pathname += "/";
		int res = dir_search(pathname);
		if (res == -1) {
			cout << "Ҫɾ�����ļ�����·��������" << endl;
			return;
		}
		DiskInode *disk = openfiletable.get_inode_table()->get_filestoremanage()->FileStoreGetDiskInode(res);
		bool is_dir;
		disk->get_dmode_file_mode(&is_dir);
		if (is_dir) {
			cout << "Ҫɾ������Ŀ¼�ļ���������ͨ�ļ���������" << endl;
			return;
		}
		/* ���ո��ļ�������File�ṹ ���ڴ�Inode �����ͷ�DiskInode */
		int find_inode = 0;
		for (int i = 0; i < openfiletable.get_max_file_cnt(); i++) {
			File *may_file = openfiletable.FGet(i);
			Inode *may_inode = may_file->get_point_inode();
			/* ��Inode�ڵ�Ϊ�գ���continue���� */
			if (may_inode == nullptr)
				continue;
			if (find_inode == 0 && may_inode->get_number() == res) {
				find_inode++;
			}
			if (find_inode == 1) {
				/* ��ѭ���н�����һ�� ���������� ʹ���ڵ���InodeReduce������ͬʱ���ͷ��ļ�DiskInode*/
				may_inode->set_nlink(may_inode->get_nlink() - 1);
				/* ������ļ����ڸ��ٻ�����Ӱ�� */
				openfiletable.get_buffermanager()->UpdateRelseBuffer(may_inode->get_index());
				find_inode++;
			}
			if (may_inode->get_number() == res) {
				openfiletable.FClose(may_file);
			}
		}
		/* ���ļ�δ�� ��û���ڴ�Inode�����DiskInode��Ӧ����� */
		if (find_inode == 0)
			openfiletable.get_inode_table()->get_filestoremanage()->FileStoreReleaseDiskInodeArray(res);
		/* ������ļ���ԴĿ¼�µ�Ŀ¼�� */
		int dir_inode = dir_search(dir_path);
		openfiletable.get_inode_table()->get_filestoremanage()->FileStoreClearDirectoryEntry(dir_inode, res);
	}

	/*
		��������: �����ļ�����ƶ��ļ�ָ��
		�������ͣ�
			fd: ������ļ����
			offset: �ƶ����ļ�ָ�����
			format: absolute ���Ծ�����ʽ   relative ��Ծ�����ʽ
	*/
	void fseek_(const int fd, const int offset, const string format)
	{
		File * pfile = openfiletable.FGet(fd);
		if (pfile->get_point_inode() == nullptr) {
			cout << "����������" << endl;
			return;
		}
		if (format == "absolute") {
			if (offset >= 0)
				pfile->set_offset(offset);
			else
				cout << "������ƶ��������" << endl;
		}
		else if (format == "relative") {
			int pos = pfile->get_offset() + offset;
			pos = pos >= 0 ? pos : 0;
			pfile->set_offset(pos);
		}
		else
			cout << "�����format��ʽ����: --absolute --relative" << endl;
	}

	/*
		��������: ����������ļ������ȡ�ļ�
		�������ͣ�
			fd: ������ļ����
			nbytes: ���ȡ���ֽ���
			output: ��ȡ�����Ľ��
		����ֵ���ͣ�ʵ�ʶ�ȡ���ֽ���
	*/
	int fread_(const int fd, const int nbytes, char *output)
	{
		File *pfile = openfiletable.FGet(fd);
		if (pfile->get_point_inode() == nullptr) {
			cout << "�ļ�����������" << endl;
			return 0;
		}
		IOParameter pread(pfile->get_offset(), nbytes);
		int actual_read = 0;
		/* ��ȡ�������������ļ���дָ��С���ļ����ȣ����Ҷ�ȡ������С��nbytes */
		while (pread.get_count() > 0 && pfile->get_point_inode()->get_size() > pread.get_offset()) {
			int lbn = pread.get_offset() / BLOCK_SIZE;
			int on = pread.get_offset() % BLOCK_SIZE;
			int cur_actual_read = min(BLOCK_SIZE - on, min(pread.get_count(), pfile->get_point_inode()->get_size() - pread.get_offset()));
			int buffer_index = openfiletable.get_buffermanager()->GetBlk(pfile->get_point_inode()->get_number(), lbn, openfiletable.get_inode_table());
			//��ƫ�Ƶ�ַ����ʼȡ�ֽ�
			memcpy(&output[actual_read], &openfiletable.get_buffermanager()->GetBufferComment(buffer_index)[on], cur_actual_read);
			/* ��ȡ��Ϻ󼴿��ͷŻ��� */
			if (openfiletable.get_buffermanager()->GetBufferByIndex(buffer_index)->get_flag() == B_DELWRI) {
				openfiletable.get_buffermanager()->Bwrite(buffer_index, openfiletable.get_inode_table());
				openfiletable.FGet(fd)->get_point_inode()->set_update(1);
			}
			else
				openfiletable.get_buffermanager()->BuffRelease(buffer_index);
			/* �޸Ķ�д���ֽ����Լ�ƫ�Ƶ�ַ */
			actual_read += cur_actual_read;
			pread.set_count(pread.get_count() - cur_actual_read);
			pread.set_offset(pread.get_offset() + cur_actual_read);
		}
		pfile->set_offset(pread.get_offset());
		return actual_read;
	}

	/*
		��������: ����������ļ����д���ļ�
		�������ͣ�
			fd: ������ļ����
			nbytes: ��д����ֽ���
			input: ��Ҫд�������
		����ֵ���ͣ�ʵ��д����ֽ���
	*/
	int fwrite_(const int fd, const int nbytes, const char *input)
	{
		/* �����ж��ļ��Ƿ���д��Ȩ�� */
		File *pfile = openfiletable.FGet(fd);
		if (pfile->get_point_inode() == nullptr) {
			cout << "�ļ�����������" << endl;
			return 0;
		}
		bool is_w, is_r;
		pfile->get_dmode_wr(&is_w, &is_r);
		if (!is_w) {
			cout << "�ļ�������д��Ȩ��" << endl;
			return 0;
		}
		IOParameter pwrite(pfile->get_offset(), nbytes);
		int actual_write = 0;
		/* д��ʱ�����ؿ��ǳ����ļ�ԭ������ */
		while (pwrite.get_count() > 0) {
			int lbn = pwrite.get_offset() / BLOCK_SIZE;
			int on = pwrite.get_offset() % BLOCK_SIZE;
			int cur_actual_write = min(BLOCK_SIZE - on, pwrite.get_count());
			int buffer_index = openfiletable.get_buffermanager()->GetBlk(pfile->get_point_inode()->get_number(), lbn, openfiletable.get_inode_table());
			memcpy(&openfiletable.get_buffermanager()->GetBufferComment(buffer_index)[on], &input[actual_write], cur_actual_write);
			/* �޸Ķ�д���ֽ����Լ�ƫ�Ƶ�ַ */
			actual_write += cur_actual_write;
			pwrite.set_count(pwrite.get_count() - cur_actual_write);
			pwrite.set_offset(pwrite.get_offset() + cur_actual_write);
			DirectoryEntry tmp;
			if (pwrite.get_offset() % BLOCK_SIZE == 0) {
				/* ����д������� ֱ�ӵ����첽д����д����� */
				openfiletable.get_buffermanager()->Bwrite(buffer_index, openfiletable.get_inode_table());
			}
			else {
				/* ��δд�����棬����B_DELWRI��ʶ���ͷż��� */
				openfiletable.get_buffermanager()->GetBufferByIndex(buffer_index)->set_flag(B_DELWRI);
				/* ��ӱ�ʶ���ͷŵ����ɶ��ж�β */
				openfiletable.get_buffermanager()->FreelistRelse(buffer_index);
			}
		}
		/* ����Inode���ļ���С �� ���±�ʶ*/
		pfile->get_point_inode()->set_size(max(pwrite.get_offset(), pfile->get_point_inode()->get_size()));
		pfile->get_point_inode()->set_update(1);
		pfile->set_offset(pwrite.get_offset());
		return actual_write;
	}

	/*
		��������: ��ȡ�ļ�ϵͳ��ǰ����Ŀ¼������
		����ֵ����: �ļ�ϵͳ��ǰ����Ŀ¼������
	*/
	string GetCurDir()
	{
		int line_pos = cur_dir_path.rfind('/');
		return cur_dir_path.substr(line_pos, -1);
	}

	OpenFileTable * Getopenfiletable()
	{
		return &this->openfiletable;
	}
};

