-- 用于监控管理系统数据库操作


-- ============================
-- 项目名称：智能监控系统
-- 数据库名称： MYSQL
-- 数据库初始化脚本

-- -- -- -- -- -- -- -- -- -- --

-- 创建用户表
-- 先删除子表，后删除父表
-- create创建表时最后一条语句不加逗号

DROP DATABASE IF EXISTS project_mission_monitor_test; -- 独立测试库
CREATE DATABASE project_mission_monitor_test


DEFAULT CHARACTER SET utf8mb4
COLLATE utf8mb4_general_ci;

USE project_mission_monitor_test;
DROP TABLE IF EXISTS exception_event;
DROP TABLE IF EXISTS image_t;
DROP TABLE IF EXISTS video_t;
DROP TABLE IF EXISTS detect_t;
DROP TABLE IF EXISTS camera_t;
DROP TABLE IF EXISTS user_t;


CREATE TABLE user_t(
  Id INT PRIMARY KEY AUTO_INCREMENT NOT NULL COMMENT'每个用户的编号',
  Username VARCHAR(30) UNIQUE NOT NULL COMMENT'每个登录用户的账户名',
  Password_user VARCHAR(50)  NOT NULL COMMENT'MD5加密后存储在表中',
  status_user TINYINT NOT NULL DEFAULT 1 COMMENT'1-可用，0-禁用',
  login_fail_count INT DEFAULT 0 COMMENT'登陆失败的次数',
  lock_until DATETIME COMMENT'登录失败多次后禁止登录的截止时间',
  created_time DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT'用户创建的时间',
  update_time DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT'用户修改时间'
);


-- 创建摄像头表
 -- DROP TABLE IF EXISTS camera_t;
CREATE TABLE camera_t(
  Id INT PRIMARY KEY AUTO_INCREMENT NOT NULL COMMENT'摄像头编号',
  Channel_id INT NOT NULL UNIQUE COMMENT'通道编号',
	Camera_index INT NOT NULL COMMENT'摄像头在opencv中的编号',
  Cameraname VARCHAR(30) NOT NULL COMMENT'摄像头名称',
  status_camera TINYINT NOT NULL DEFAULT 1 COMMENT'1-在线，0-离线',
	cerate_time DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT'摄像头创建时间',
	update_time DATETIME DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP COMMENT'摄像头更新时间'
);

-- 创建普通视频表
-- DROP TABLE IF EXISTS video_t;
CREATE TABLE video_t(
	Id INT AUTO_INCREMENT PRIMARY KEY NOT NULL COMMENT'视频编号',
	Camera_id INT NOT NULL COMMENT'关联的摄像头编号',
	file_size BIGINT NOT NULL DEFAULT 0 COMMENT'视频文件大小',
	Videoname VARCHAR(100) NOT NULL COMMENT'视频名称',
	Video_path VARCHAR(200) NOT NULL COMMENT'视频保存路径',
	start_time DATETIME DEFAULT CURRENT_TIMESTAMP NOT NULL COMMENT'视频开始时间',
	end_time DATETIME DEFAULT CURRENT_TIMESTAMP NOT NULL COMMENT'视频结束时间',
	create_time DATETIME DEFAULT CURRENT_TIMESTAMP  NOT NULL COMMENT'视频创建时间',
	-- 关联外键
	FOREIGN KEY(Camera_id) 
	REFERENCES camera_t(Id)
);


-- 创建移动侦测视频表
CREATE TABLE detect_t(

	Id INT AUTO_INCREMENT PRIMARY KEY NOT NULL COMMENT'视频编号',
	Camera_id INT NOT NULL COMMENT'关联的摄像头编号',
	file_size BIGINT NOT NULL DEFAULT 0 COMMENT'视频文件大小',
	Videoname VARCHAR(100) NOT NULL COMMENT'视频名称',
	Video_path VARCHAR(200) NOT NULL COMMENT'视频保存的路径',
	start_time DATETIME DEFAULT CURRENT_TIMESTAMP NOT NULL COMMENT'视频开始时间',
	end_time DATETIME DEFAULT CURRENT_TIMESTAMP NOT NULL COMMENT'视频结束时间',
	create_time DATETIME DEFAULT CURRENT_TIMESTAMP NOT NULL COMMENT'视频创建时间',

-- 关联外键
	FOREIGN KEY(Camera_id)
	REFERENCES camera_t(Id) 
);

-- 创建图片表
-- DROP TABLE IF EXISTS image_t;
CREATE TABLE image_t(
	Id INT PRIMARY KEY NOT NULL AUTO_INCREMENT COMMENT'视频编号',
	Camera_id INT NOT NULL COMMENT'关联的摄像头编号',
	Video_id INTEGER NOT NULL COMMENT'关联的视频编号',
	Imagename VARCHAR(100) NOT NULL COMMENT'图片名称',
	Image_size BIGINT DEFAULT 0 NOT NULL COMMENT'图片文件大小',
	Image_path VARCHAR(200) NOT NULL COMMENT'图片保存的路径',
	Create_time DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT'图片的创建/截图时间',
	event_type TINYINT COMMENT'图片类型，1-自动截取(异常截取)，0-手动截取',
	-- 关联外键
	FOREIGN KEY(Camera_id) 
	REFERENCES camera_t(Id),
	
	FOREIGN KEY(Video_id) 
	REFERENCES video_t(Id)
);

-- 创建异常事件表

-- DROP TABLE IF EXISTS exception_event;
 CREATE TABLE exception_event(
 Id BIGINT PRIMARY KEY NOT NULL AUTO_INCREMENT	COMMENT'异常事件编号',
	camera_id INT NOT NULL COMMENT'关联的摄像头编号',
	video_id INT NOT NULL COMMENT'关联的视频编号',
	event_type TINYINT NOT NULL COMMENT'1-人体检测,0-其他物体检测',
	triger_time DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT'事件的触发时间',
	image_count INT NOT NULL COMMENT'异常图片的数量',
	description VARCHAR(300) COMMENT'事件描述',
	create_time DATETIME DEFAULT CURRENT_TIMESTAMP COMMENT'创建时间',
	
	-- 关联外键
	FOREIGN KEY(camera_id)
	REFERENCES camera_t(Id),
	
	FOREIGN KEY(video_id)
	REFERENCES video_t(Id)
);

INSERT INTO user_t(Username,Password_user) VALUES('admin',MD5('123456'));
-- 创建默认用户admin

