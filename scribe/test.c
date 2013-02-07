#include <stdio.h>
#include <stdlib.h>
#include "python/python.h"

int main(int argc, char** argv)
{
    // 初始化Python
    //在使用Python系统前，必须使用Py_Initialize对其
    //进行初始化。它会载入Python的内建模块并添加系统路
    //径到模块搜索路径中。这个函数没有返回值，检查系统
    //是否初始化成功需要使用Py_IsInitialized。

	PyObject *pModule, *pDict, *pScriberClient, *pScriberClientClass;  
    Py_Initialize();
    // 检查初始化是否成功
    if ( !Py_IsInitialized() ) 
    {
        return -1;
    }

	PyRun_SimpleString("import sys");   
	PyRun_SimpleString("sys.path.append('./py/')"); 
	pModule = PyImport_ImportModule("ScribeClient"); 
	if (!pModule) {  
		printf("can't find ScribeClient.py");  
		getchar();
		return -1;  
	} 
	pDict = PyModule_GetDict(pModule);  
	if (!pDict) {
		return -1;  
	}  
	pScriberClientClass = PyDict_GetItemString(pDict, "Scriber");
	if (!pScriberClientClass || !PyClass_Check(pScriberClientClass)) {
	   printf("w111\n"); 		
		return -1;  
	}

	PyObject *pArgs = PyTuple_New(3);
	PyTuple_SetItem(pArgs, 0, Py_BuildValue("s","172.16.111.132"));
	PyTuple_SetItem(pArgs, 1, Py_BuildValue("s","1463"));
	PyTuple_SetItem(pArgs, 2, Py_BuildValue("b","False"));
	pScriberClient = PyInstance_New(pScriberClientClass, pArgs, NULL);
	 
	if (!pScriberClient) { 
		return -1;  
	}
	PyObject_CallMethod(pScriberClient, "slog", "(ss)", "default", "c调用python test");

    Py_DECREF(pScriberClientClass);
    Py_DECREF(pScriberClient);
    Py_DECREF(pModule);

    // 关闭Python
    Py_Finalize();
    return 0;
}


