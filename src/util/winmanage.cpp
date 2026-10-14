#include "winmanage.h"

//初始化静态变量
winmanage* winmanage::one_instance;
winmanage *winmanage::Getinstance()
{
    if(!winmanage::one_instance){
        winmanage::one_instance=new winmanage;
    }
    return winmanage::one_instance;
}


winmanage::winmanage() {

}
