 ///
 /// @file    MyTask.cc
 /// @author  lemon(haohb13@gmail.com)
 /// @date    2016-03-28 15:34:24
 ///

#include "MyTask.h"
#include "MyDict.h"
#include "Cache.h"
#include "EditDistance.h"
#include <stdio.h>
#include <unistd.h>
#include <iostream>

using std::cout;
using std::endl;



MyTask::MyTask(const string & queryWord, int peerfd)
: _queryWord(queryWord)
, _peerfd(peerfd)
{
}

void MyTask::execute(Cache & cache)
{//每一个任务都是通过某一个线程来执行的，所以这里的cache
 //是由线程传过来的
#if 0
	cout << "Task::execute()--->query " << _queryWord << endl;
	MyDict * pDict = MyDict::createInstance();
	auto myDict = pDict->get_dict();
	cout << myDict[0].first << "-->" << myDict[0].second << endl;
#endif
	//1. 从cache里面进行查找
	string result = cache.query(_queryWord);
	if(result != string())
	{
		::write(_peerfd, result.c_str(), result.size());
		cout << "response client" << endl;
		return;
	}

	queryIndexTable();//2. 执行查询,查询索引表

	response(cache);//3. 给客户端返回结果
}


void MyTask::queryIndexTable()
{
	auto indexTable = MyDict::createInstance()->get_index_table();
	string ch;

	for(int idx = 0; idx != _queryWord.size();)
	{
		size_t nBytes = nBytesCode(_queryWord[idx]);
		ch = _queryWord.substr(idx, nBytes);
		idx += nBytes;
		if(indexTable.count(ch))
		{
			cout << "indexTable has character " << ch << endl;
			statistic(indexTable[ch]);
		}
	}
}

void MyTask::response(Cache & cache)
{
	if(_resultQue.empty())
	{
		string result = "no answer!";
		int nwrite = ::write(_peerfd, result.c_str(), result.size());
		if(-1 == nwrite)
		{
			cout << "reponse error" << endl;
		}
	}
	else
	{
		MyResult result = _resultQue.top();
		int nwrite = ::write(_peerfd, result._word.c_str(), result._word.size());
		if(-1 == nwrite)
		{
			cout << "reponse error" << endl;
		}
		cache.addElement(_queryWord, result._word);//更新缓存
		cout << "respone(): add Cache" << endl;
	}
	cout << "reponse client" << endl;
}

void MyTask::statistic(set<int> & iset)
{
	auto dict = MyDict::createInstance()->get_dict();
	auto iter = iset.begin();
	for(; iter != iset.end(); ++iter)
	{
		string rhsWord = dict[*iter].first;
		int idist = distance(rhsWord);//进行最小编辑距离的计算
		if(idist < 3)
		{
			MyResult res;
			res._word = rhsWord;
			res._iFreq = dict[*iter].second;
			res._iDist = idist;
			_resultQue.push(res);
		}
	}
}

int MyTask::distance(const string & rhsWord)
{
	return ::editDistance(_queryWord, rhsWord);
}
