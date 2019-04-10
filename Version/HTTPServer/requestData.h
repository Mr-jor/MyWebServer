#ifndef REQUESTDATA
#define REQUESTDATA
#include <string>
#include <unordered_map>

using namespace std ; 

const int STATE_PARSE_URI = 1 ;
const int STATE_PARSE_HEADERS = 2 ;
const int STATE_RECV_BODY = 3 ;
const int STATE_ANALYSIS = 4 ;
const int STATE_FINISH = 5 ;

const int MAX_BUFF = 4096 ;

// ��������ֵ��Ƕ���������,������Request Aborted,
// �����������������û�дﵽ��ԭ��,
// �������������Գ���һ���Ĵ���������
const int AGAIN_MAX_TIMES = 200;

const int PARSE_URI_AGAIN = -1;
const int PARSE_URI_ERROR = -2;
const int PARSE_URI_SUCCESS = 0;

const int PARSE_HEADER_AGAIN = -1;
const int PARSE_HEADER_ERROR = -2;
const int PARSE_HEADER_SUCCESS = 0;

const int ANALYSIS_ERROR = -2;
const int ANALYSIS_SUCCESS = 0;

const int METHOD_POST = 1;
const int METHOD_GET = 2;
const int HTTP_10 = 1;
const int HTTP_11 = 2;

const int EPOLL_WAIT_TIME = 500;

class MimeType
{
private:
	static pthread_mutex_t lock ;
	static unordered_map<string,string> mime ;
	MimeType() ;
	MimeType(const MimeType &m) ;
public:
	static string getMime(const string &suffix) ;
} ;

enum HeadersState
{
	h_start = 0 ,
	h_key,
	h_colon,
	h_spaces_after_colon,
	h_value,
	h_CR,
	h_LF,
	h_end_CR,
	h_end_LF
};

struct mytimes;
struct requestData ;

struct requestData
{
private:
	/* Epoll�����¼���Ҫ������ */
	int fd ;
	int epollfd;

	/* ������Ҫ�õ������� */
	int againTimes ;
	string path ;
	int now_read_pos;
	int state ;
	int h_state ;
	bool isfinish ;

	/* ����http����Ľ�� */
	//content�����������Ҫ���
	string content ;
	int method ;
	int HTTPversion ;
	string file_name ;
	bool keep_alive ;
	unordered_map<string,string> headers ;
	mytimer *timer ;

private:
	int parse_URI() ;
	int parse_Headers() ;
	int analysisRequest() ;

public:
	requestData() ;
	requestData(int _epollfd, int _fd, string _path) ;
	~requestData() ;
	void addTimer(mytimer *mtimer) ;
	void reset() ;
	void seperateTimer() ;
	int getFd() ;
	void setFd(int _fd) ;
	void handleRequest() ;
	void handleError(int fd, int err_num, string short_msg) ;
};


struct mytimer
{
	bool deleted ;
	size_t expired_time ;
	requestData *request_data ;

	mytimer(requestData *request_data, int timeout) ;
	~mytimer() ;
	void update(int timeout) ;
	bool isvalid() ;
	void clearReq() ;
	void setDeleted() ;
	bool isDeleted() const;
	size_t getExpTime() const ;
};

struct timerCmp
{
	bool operator()(const mytimer *a, const mytimer *b) const ;
};



#endif