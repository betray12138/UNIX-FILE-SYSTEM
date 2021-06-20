#include "FileOpenManager.h"
#include <stdio.h>
/*
	类声明：文件管理系统顶层类，用于提供各种函数调用接口
	成员变量解释：
		cur_dir_path: 存储用户当前所在目录的根路径
		cur_dir_diskinode: 使用可变数组来存储从根路径到达当前目录DiskInode的Inode序号  便于后退
		root_inode_index: 根节点目录文件的DiskInode序号
		openfiletable: 用于管理系统的打开文件表
	功能解释：管理内存所有打开文件
*/
class FileSys {
	string cur_dir_path;
	vector<int> cur_dir_diskinode;
	int root_inode_index;
	OpenFileTable openfiletable;

public:
	FileSys()
	{
		/* 程序开始时，处于根目录下 */
		cur_dir_path = "/";
		// 从文件中读取root_inode_index
		fp.seekg(0, ios::beg);
		fp.read((char *)&root_inode_index, sizeof(int));
		if (root_inode_index == 0) {
			// 未读取到值 则进行初始化
			/* 为根目录申请一个diskInode */
			openfiletable.get_inode_table()->get_filestoremanage()->format();
			root_inode_index = openfiletable.get_inode_table()->get_filestoremanage()->FileStoreInodeAlloc();
			DiskInode *root_inode = openfiletable.get_inode_table()->get_filestoremanage()->FileStoreGetDiskInode(root_inode_index);
			root_inode->set_dmode_file_mode(true);
			// 将root_index写入文件
			fp.seekg(0, ios::beg);
			fp.write((char *)&root_inode_index, sizeof(int));
		}
		cur_dir_diskinode.push_back(root_inode_index);
	}

	/*
		函数名字: 根据输入的路径 寻找到其最后一级目录结构 若无法找到返回-1 否则返回
		参数解释：
			dir_path: 接受绝对路径和相对路径两种形式，要求绝对路径以/开头， 相对路径以./开头 且路径末尾不出现/
					  路径中不允许出现两个连续的/
			reg: 是否需要登记进入cur_dir_diskinode数组中
		返回值解释：若可以进入最后一级目录，则返回目录的DiskInode序号，否则返回-1表示无法找到
	*/
	int dir_search(string dir_path, bool reg = false)
	{
		int cur_inode_index = root_inode_index;
		/* 对于相对路径 将其转化为绝对路径即可 不同表现在cur_inode_index，其余应该相同*/
		if (dir_path[0] != '/') {
			/* 相对路径形式 仅允许开始出现../的情况，不允许中间出现../的情况 */
			if (dir_path[0] != '.') {
				/* 如果相对路径不是./...的形式， 则在最前面加上./ */
				dir_path = "./" + dir_path;
			}
			/* 使用backwards_cnt 记录后退次数*/
			int backwards_cnt = 0;
			int normal_pos = 0;
			for (int i = 0; i < dir_path.size() - 1; i++) {
				if (dir_path[i] == '.' && dir_path[i + 1] == '.') {
					backwards_cnt++;
					normal_pos = i + 2;   // normal_pos 用于标记最后一次../中/出现的位置
				}
				else if (dir_path[i] == '.' && dir_path[i + 1] == '/') {
					/* 针对路径中仅有./ 出现 没有../出现的情况 */
					normal_pos = i + 1;
				}
			}
			dir_path = dir_path.substr(normal_pos, -1);
			int pos = max((int)cur_dir_diskinode.size() - 1 - backwards_cnt, 0);
			cur_inode_index = cur_dir_diskinode[pos];
			/*对于要登记的情况，需要将数组进行回退, 将string进行减少*/
			while (reg == true && cur_dir_diskinode[cur_dir_diskinode.size() - 1] != cur_inode_index) {
				cur_dir_diskinode.pop_back();
				int last_line_pos = cur_dir_path.rfind('/', cur_dir_path.size());
				cur_dir_path = cur_dir_path.substr(0, last_line_pos);
			}
		}
		/* 处理绝对路径 且需要登记的情况 */
		else if(reg == true){
			/* 将cur_dir_diskinode 全部pop 并且cur_dir_path 清空*/
			cur_dir_path = "/";
			cur_dir_diskinode.clear();
			cur_dir_diskinode.push_back(root_inode_index);
		}
		/* 处理绝对路径 */
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
				/* 对于要登记的情况，需要将数组添加，将string进行增长*/
				if (reg == true) {
					cur_dir_diskinode.push_back(next_layer_index);
					cur_dir_path += string("/") + match_dir_name;
				}
			}
		}
		return cur_inode_index;
	}

	/*
		函数名字: 创建文件
		参数解释：
			filename: 文件的名字
			is_w： 文件的读写权限 0 -- 只读 1 -- 读写
		返回值解释：false 表示创建失败   true 表示创建成功
	*/
	bool fcreate(string filename, const bool is_w)
	{
		int diskinode_index = dir_search(filename);
		if (diskinode_index == -1) {
			cout << "不存在对应目录" << endl;
			return false;
		}

		/* 将所需创建的文件名从file_name中取出*/
		int line_pos = -1;
		for (int i = 0; i < filename.size(); i++)
			if (filename[i] == '/')
				line_pos = i;

		filename = filename.substr(line_pos + 1, -1);
		/* 最终的filename应该类似于 ab.txt */
		/* 在当前目录下对filename 进行搜索， 若存在filename 则报错 若不存在 则创建filename 并且登记进diskinode_index中*/
		int file_inode_index = openfiletable.get_inode_table()->get_filestoremanage()->FileStoreGetDiskInode(diskinode_index)->dir_path_match(filename);
		if (file_inode_index != -1) {
			/* 文件存在*/
			cout << "当前目录或文件已存在，无法再次创建" << endl;
			return false;
		}
		else {
			/* 文件不存在 */
			/* 为新文件创建一个DiskInode，并且登记到目录的diskInode中 */
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
		函数名字: 创建新目录
		参数解释：
			dirname：新目录的名字
		返回值解释：false 表示创建失败   true 表示创建成功
	*/
	bool mkdir(string dirname)
	{
		int diskinode_index = dir_search(dirname);
		if (diskinode_index == -1) {
			cout << "不存在对应目录" << endl;
			return false;
		}

		/* 将所需创建的文件名从file_name中取出*/
		int line_pos = -1;
		for (int i = 0; i < dirname.size(); i++)
			if (dirname[i] == '/')
				line_pos = i;

		dirname = dirname.substr(line_pos + 1, -1);
		/* 最终的dirname应该类似于 root */
		/* 在当前目录下对dirname 进行搜索， 若存在dirname 则报错提示 若不存在 则创建dirname 并且登记进diskinode_index中*/
		int file_inode_index = openfiletable.get_inode_table()->get_filestoremanage()->FileStoreGetDiskInode(diskinode_index)->dir_path_match(dirname);
		if (file_inode_index != -1) {
			/* 目录存在*/
			cout << "当前目录或文件已存在，无法再次创建" << endl;
			return false;
		}
		else {
			/* 目录不存在 */
			/* 为新目录创建一个DiskInode，并且登记到源目录的diskInode中 */
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
		函数名字: 查看输入路径的文件和目录信息
		参数解释：
			path_name: 所要查看的路径名字
	*/
	void ls(string path_name = "")
	{
		/* 若ls没有输入参数，直接显示当前目录下文件即可 */
		int dir_path_inode_index = cur_dir_diskinode[cur_dir_diskinode.size() - 1];

		if (path_name != "") {
			path_name = (path_name[path_name.size() - 1] == '/' ? path_name : path_name + "/");
			int res = dir_search(path_name);
			if (res == -1) {
				cout << "输入的路径不存在，请重新输入" << endl;
				return;
			}
			else
				dir_path_inode_index = res;
		}
		DiskInode *dir_disk = openfiletable.get_inode_table()->get_filestoremanage()->FileStoreGetDiskInode(dir_path_inode_index);
		dir_disk->print_dir_item();
	}

	/*
		函数名字: 进入某个目录页面下
		参数解释：
			path_name: 所要进入的目录路径  若该路径结果是文件  则无法进入
	*/
	void cd(string path_name)
	{
		if (path_name[path_name.size() - 1] != '/')
			path_name += "/";
		int res = dir_search(path_name);
		if (res == -1) {
			cout << "该目录不存在， 无法进入该目录" << endl;
			return;
		}
		DiskInode *disk = openfiletable.get_inode_table()->get_filestoremanage()->FileStoreGetDiskInode(res);
		bool is_dir;
		disk->get_dmode_file_mode(&is_dir);
		if (!is_dir) {
			cout << "所需进入的目录实质是一个普通文件，无法进入" << endl;
			return;
		}
		dir_search(path_name, true);
	}

	/*
		函数名字: 打开文件
		参数解释：
			filename: 文件的名字
			is_w： 文件的读写权限 0 -- 只读 1 -- 读写
		返回值解释：返回文件句柄，若返回-1则打开失败
	*/
	int fopen(string filename, const bool is_w)
	{
		if (filename[filename.size() - 1] != '/')
			filename += "/";
		int file_diskinode_index = dir_search(filename);
		if (file_diskinode_index == -1) {
			cout << "文件不存在，打开失败" << endl;
			return -1;
		}
		Inode *file_inode = openfiletable.get_inode_table()->InodeGetWithIndexOfDiskInode(file_diskinode_index);
		bool wr, is_r, is_dir;
		file_inode->get_imode_dir(&is_dir);
		if (is_dir) { 
			cout << "该文件是目录文件，无法被打开" << endl;
			openfiletable.get_inode_table()->InodeReduce(file_inode);
			return -1;
		}
		file_inode->get_imode_wr(&wr, &is_r);

		if (wr < is_w) {
			cout << "文件没有写入权限，打开失败" << endl;
			openfiletable.get_inode_table()->InodeReduce(file_inode);
			return -1;
		}
		/* 为文件的内存Inode申请一个新的打开文件结构*/
		int fd;
		File * fileopen = openfiletable.FAlloc(fd);
		fileopen->set_count(1);
		fileopen->set_imode_wr(is_w, true);
		fileopen->set_offset(0);
		fileopen->set_point_inode(file_inode);
		return fd;
	}

	/*
		函数名字: 关闭文件
		参数解释：
			fd: 文件句柄
		功能解释： 根据文件句柄关闭文件
	*/
	void fclose(const int fd)
	{
		File * pfile = openfiletable.FGet(fd);
		if (pfile->get_point_inode() == nullptr) {
			cout << "句柄输入错误" << endl;
			return;
		}
		openfiletable.FClose(pfile);
	}

	/*
		函数名字: 删除文件
		参数解释：
			pathname: 要删除的文件
	*/
	void fdelete(string pathname)
	{
		string dir_path = pathname;
		if (pathname[pathname.size() - 1] != '/')
			pathname += "/";
		int res = dir_search(pathname);
		if (res == -1) {
			cout << "要删除的文件或者路径不存在" << endl;
			return;
		}
		DiskInode *disk = openfiletable.get_inode_table()->get_filestoremanage()->FileStoreGetDiskInode(res);
		bool is_dir;
		disk->get_dmode_file_mode(&is_dir);
		if (is_dir) {
			cout << "要删除的是目录文件，而是普通文件，请重试" << endl;
			return;
		}
		/* 回收该文件的所有File结构 和内存Inode 并且释放DiskInode */
		int find_inode = 0;
		for (int i = 0; i < openfiletable.get_max_file_cnt(); i++) {
			File *may_file = openfiletable.FGet(i);
			Inode *may_inode = may_file->get_point_inode();
			/* 若Inode节点为空，则continue即可 */
			if (may_inode == nullptr)
				continue;
			if (find_inode == 0 && may_inode->get_number() == res) {
				find_inode++;
			}
			if (find_inode == 1) {
				/* 在循环中仅设置一次 链接数减少 使得在调用InodeReduce函数的同时，释放文件DiskInode*/
				may_inode->set_nlink(may_inode->get_nlink() - 1);
				/* 清除该文件对于高速缓存块的影响 */
				openfiletable.get_buffermanager()->UpdateRelseBuffer(may_inode->get_index());
				find_inode++;
			}
			if (may_inode->get_number() == res) {
				openfiletable.FClose(may_file);
			}
		}
		/* 若文件未打开 即没有内存Inode与外存DiskInode对应的情况 */
		if (find_inode == 0)
			openfiletable.get_inode_table()->get_filestoremanage()->FileStoreReleaseDiskInodeArray(res);
		/* 清除该文件在源目录下的目录项 */
		int dir_inode = dir_search(dir_path);
		openfiletable.get_inode_table()->get_filestoremanage()->FileStoreClearDirectoryEntry(dir_inode, res);
	}

	/*
		函数名字: 根据文件句柄移动文件指针
		参数解释：
			fd: 输入的文件句柄
			offset: 移动的文件指针距离
			format: absolute 绝对距离形式   relative 相对距离形式
	*/
	void fseek_(const int fd, const int offset, const string format)
	{
		File * pfile = openfiletable.FGet(fd);
		if (pfile->get_point_inode() == nullptr) {
			cout << "句柄输入错误" << endl;
			return;
		}
		if (format == "absolute") {
			if (offset >= 0)
				pfile->set_offset(offset);
			else
				cout << "输入的移动距离错误" << endl;
		}
		else if (format == "relative") {
			int pos = pfile->get_offset() + offset;
			pos = pos >= 0 ? pos : 0;
			pfile->set_offset(pos);
		}
		else
			cout << "输入的format格式错误: --absolute --relative" << endl;
	}

	/*
		函数名字: 根据输入的文件句柄读取文件
		参数解释：
			fd: 输入的文件句柄
			nbytes: 需读取的字节数
			output: 读取出来的结果
		返回值解释：实际读取的字节数
	*/
	int fread_(const int fd, const int nbytes, char *output)
	{
		File *pfile = openfiletable.FGet(fd);
		if (pfile->get_point_inode() == nullptr) {
			cout << "文件句柄输入错误" << endl;
			return 0;
		}
		IOParameter pread(pfile->get_offset(), nbytes);
		int actual_read = 0;
		/* 读取的条件是满足文件读写指针小于文件长度，并且读取的数量小于nbytes */
		while (pread.get_count() > 0 && pfile->get_point_inode()->get_size() > pread.get_offset()) {
			int lbn = pread.get_offset() / BLOCK_SIZE;
			int on = pread.get_offset() % BLOCK_SIZE;
			int cur_actual_read = min(BLOCK_SIZE - on, min(pread.get_count(), pfile->get_point_inode()->get_size() - pread.get_offset()));
			int buffer_index = openfiletable.get_buffermanager()->GetBlk(pfile->get_point_inode()->get_number(), lbn, openfiletable.get_inode_table());
			//从偏移地址处开始取字节
			memcpy(&output[actual_read], &openfiletable.get_buffermanager()->GetBufferComment(buffer_index)[on], cur_actual_read);
			/* 读取完毕后即可释放缓存 */
			if (openfiletable.get_buffermanager()->GetBufferByIndex(buffer_index)->get_flag() == B_DELWRI) {
				openfiletable.get_buffermanager()->Bwrite(buffer_index, openfiletable.get_inode_table());
				openfiletable.FGet(fd)->get_point_inode()->set_update(1);
			}
			else
				openfiletable.get_buffermanager()->BuffRelease(buffer_index);
			/* 修改读写的字节数以及偏移地址 */
			actual_read += cur_actual_read;
			pread.set_count(pread.get_count() - cur_actual_read);
			pread.set_offset(pread.get_offset() + cur_actual_read);
		}
		pfile->set_offset(pread.get_offset());
		return actual_read;
	}

	/*
		函数名字: 根据输入的文件句柄写入文件
		参数解释：
			fd: 输入的文件句柄
			nbytes: 需写入的字节数
			input: 需要写入的数据
		返回值解释：实际写入的字节数
	*/
	int fwrite_(const int fd, const int nbytes, const char *input)
	{
		/* 首先判断文件是否有写入权限 */
		File *pfile = openfiletable.FGet(fd);
		if (pfile->get_point_inode() == nullptr) {
			cout << "文件句柄输入错误" << endl;
			return 0;
		}
		bool is_w, is_r;
		pfile->get_dmode_wr(&is_w, &is_r);
		if (!is_w) {
			cout << "文件不具有写入权限" << endl;
			return 0;
		}
		IOParameter pwrite(pfile->get_offset(), nbytes);
		int actual_write = 0;
		/* 写入时，不必考虑超出文件原本长度 */
		while (pwrite.get_count() > 0) {
			int lbn = pwrite.get_offset() / BLOCK_SIZE;
			int on = pwrite.get_offset() % BLOCK_SIZE;
			int cur_actual_write = min(BLOCK_SIZE - on, pwrite.get_count());
			int buffer_index = openfiletable.get_buffermanager()->GetBlk(pfile->get_point_inode()->get_number(), lbn, openfiletable.get_inode_table());
			memcpy(&openfiletable.get_buffermanager()->GetBufferComment(buffer_index)[on], &input[actual_write], cur_actual_write);
			/* 修改读写的字节数以及偏移地址 */
			actual_write += cur_actual_write;
			pwrite.set_count(pwrite.get_count() - cur_actual_write);
			pwrite.set_offset(pwrite.get_offset() + cur_actual_write);
			DirectoryEntry tmp;
			if (pwrite.get_offset() % BLOCK_SIZE == 0) {
				/* 缓存写满的情况 直接调用异步写操作写入磁盘 */
				openfiletable.get_buffermanager()->Bwrite(buffer_index, openfiletable.get_inode_table());
			}
			else {
				/* 若未写满缓存，则置B_DELWRI标识后释放即可 */
				openfiletable.get_buffermanager()->GetBufferByIndex(buffer_index)->set_flag(B_DELWRI);
				/* 添加标识后释放到自由队列队尾 */
				openfiletable.get_buffermanager()->FreelistRelse(buffer_index);
			}
		}
		/* 更新Inode中文件大小 和 更新标识*/
		pfile->get_point_inode()->set_size(max(pwrite.get_offset(), pfile->get_point_inode()->get_size()));
		pfile->get_point_inode()->set_update(1);
		pfile->set_offset(pwrite.get_offset());
		return actual_write;
	}

	/*
		函数名字: 获取文件系统当前所在目录的名字
		返回值解释: 文件系统当前所在目录的名字
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

