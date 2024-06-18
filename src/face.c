#include <Python.h>


//为什么能用 127.0.0.1  我们是vscode 远程连接到香橙派 --  实际上是在orangepi上运行，用本地ip即可
#define WGET_CMD "wget http://127.0.0.1:8080/?action=snapshot -O/tmp/SearchFace.jpg"
#define SEARCHFACE_FILE "/tmp/SearchFace.jpg"


void face_init(void)
{
	Py_Initialize();
	PyObject *sys = PyImport_ImportModule("sys");

	PyObject *path = PyObject_GetAttrString(sys, "path");

	PyList_Append(path, PyUnicode_FromString("."));
}

void face_final(void)
{
	Py_Finalize();
}


double face_status(void)
{
	double result = 0.0;
	system(WGET_CMD); // 拍照
    if(-1 == access(SEARCHFACE_FILE,F_OK)) // 打开这个文件失败--> 文件不存在,返回 -1
	{
		return result;
	}


	// 加载face.py 这个文件
	PyObject *pModule = PyImport_ImportModule("face");
	if (!pModule)
	{
		PyErr_Print();
		printf("Error: failed to load face.py\n");
		goto FAILED_MODULE;
	}
	// 加载(获取) alibaba_face() 这个函数 -- 顺便判断是否存在
	PyObject *pfunc = PyObject_GetAttrString(pModule, "alibaba_face");

	if (!pfunc)
	{
		PyErr_Print();
		printf("Error: failed to load alibaba_face\n");
		goto FAILED_FUNC;
	}

	// 调用我们获取到的函数
	PyObject *pValue = PyObject_CallObject(pfunc, NULL);

	if (!pValue)
	{
		PyErr_Print();
		printf("Error:function call failed\n");
		goto FAILED_VALUE;
	}

	
	if (!PyArg_Parse(pValue, "d", &result)) // 解析获取 alibaba_face() 返回值， 转换成C语言格式
	{

		PyErr_Print();
		printf("Error:face failed\n");
		goto FAILED_RESULT;
	}
	printf("result=%.2lf\n", result);

	// 注意释放顺序是反的，从新到旧
	//  添加跳转位置，当发生错误的时候，跳转过来将他释放
FAILED_RESULT:
	Py_DECREF(pValue);

FAILED_VALUE:
	Py_DECREF(pfunc);

FAILED_FUNC:
	Py_DECREF(pModule);

FAILED_MODULE:

	return result;
}
