# 应用于足球比赛视频的多目标检测与信息统计系统

## 项目简介

本项目是一个实现了对足球视频中的运动员进行识别、追踪，通过童虎交互系统展示，并可以在用户交互系统中查看统计信息、调整识别结果的项目。

其中识别追踪部分参考了https://github.com/mikel-brostrom/Yolov5_DeepSort_Pytorch，交互系统部分全部由项目组进行设计、开发。

**项目适用输入：**固定视角的双目足球比赛视频 

## 使用方法

### 识别结果输出

Windows平台下，进入`Yolov5_DeepSort`文件夹，运行run.bat即可生成关于left.mp4和right.mp4的，如果要识别别的视频或是在其它平台上使用，可以参考开源代码https://github.com/mikel-brostrom/Yolov5_DeepSort_Pytorch中的教程。

其中，为了适配交互系统，输出txt中每行的格式为，`id, frame, cls, left, top, width, height, confidence, x, y, z`，其中后四项暂时未在交互系统中使用。本项目中的deepsort部分已对输出格式进行更改，直接使用开源代码则需注意。

### 交互系统使用

#### 交互系统概览

<img src=".\pic\10.png" alt="10" style="zoom:40%;" />

用户交互界面主界面的整体布局如图所示，界面按左右分割为视频展示区和操控区。

其中界面左边是视频展示区，主要用于各种图像可视化的展示，上半部分分左右两栏同步播放来自两个固定机位的视频，左下角为足球场的二维平面图，将展示运动员位置的二维投影，右下角可以通过选择框切换维局部放大和热力图，选择框选项切换到’detail:’，将放大显示所选运动员的所区域。切换到’heatmap:’，将显示所有运动员的热力图。

界面右半部分为操控区。可以通过点击上方标签页在信息展示以及标注修改面板中进行切换。其中标注修改面板如图10所示，可以加载标注文件，修改标签信息等。而信息展示面板则在图9中展示，可以添加球员个人信息，查看球员的运动速度、平均速度、最高速度、运动距离的实时信息，以及运动速度和运动距离的图表显示。

#### 视频载入与播放

视频播放器支持包括mp4、avi等格式的视频播放。

<img src=".\pic\11.png" alt="11" style="zoom:75%;" />

如图展示了交互界面中的视频播放控件，开始时点击“load video”按钮，可以依次选择文件加载需要播放的左右视角视频。进度条左侧的播放按钮可以控制视频的播放和停止。因为运动员信息数组存储进度条暂时不支持拖动。但随着视频的播放，进度条的位置可以显示视频的播放进度。

##### 标准足球场配准

![12](.\pic\12.png)

在加载视频的过程中，每当一个视频选择完毕，系统会自动跳出对话框要求用户配准标准足球场，如图所示。

配准的方式是拖动图中绿色边框的角点，使黑色足球边框的边界对准视频中足球场的位置。系统能在这个过程中获得视频到标准足球场的变化矩阵，如3.3节中所提到的那样。因为系统输入的要求是两个固定视角的半场足球比赛视频，所以获得的变换矩阵适用于整个视频。

#### 个人信息展示

将主界面的操控区切换到“information”模式，可以在控制区编辑球员的个人基本信息，查看球员的统计信息，还有一些诸如运动轨迹的信息也在交互界面的其它部分展示。

##### 个人信息添加

![13](.\pic\13.png)

在统计信息输出面板，用户可以选择当前想要展示的球员id编号，在下方的”Basic Information”模块中，可以编辑选中球员的基本信息，包括球员姓名，球员队伍，球衣号码。其中队伍信息默认是\<none\>，用户通过下拉菜单，可以选择之前添加过的队伍，或者选择\<add new\>，在跳出的对话框中添加新的队伍。

添加的信息会被保存，在修改标签时，会显示已添加的球员信息，方便用户辨认id与球员之间的对应关系，在执行merge操作时也会展示球员的信息。

##### 个人统计信息展示

![14a](.\pic\14-a.png)

在统计信息输出面板的下方”statistic information”模块，会随着视频的播放输出选中运动员的实时统计信息，包括实时速度、最大速度、平均速度、累积运动距离。由于距离与速度随着时间变化，因此也绘制了这两个信息随着时间的变化图，方便用户查看。通过点击图表标签页上方的”distance”和“speed”，可以在速度曲线与运动曲线之间变换。

##### 个人轨迹展示

![14b](.\pic\14-b.png)

在选中某个球员id的情况下，会在视频播放区域左下角的二维展示部分展示选中球员的运动轨迹，如图所示，展示该球员从开始到目前帧的选中轨迹。

##### 运动员局部放大

![14c](.\pic\14-c.png)

如果选中了某个运动员并且在视频播放区的右下角下拉菜单中选择了”detail”，那么会在下方显示选中运动员所在区域的局部放大画面。放大后的区域中标注框处在画面的正中央。

##### id融合

![15a](.\pic\15-b.png)

因为输入是左右两个半场的视频，在项目测试过程中，项目组成员发现存在有左半场的球员运动到有半场，便变化了一个id，于是它们拥有各自的统计信息，不便于观察。因此在开发过程中增加了一个融合功能，点击的”merge with…”按钮即可将选中球员的信息与某一个球员的信息融合。另一个球员的选中在如图15.b所示的对话框中进行，在下拉菜单中选择不同的球员，会在右侧显示该球员的个人信息以及截取的画面，方便确认是否需要融合的是这个球员。

![15b](.\pic\15-a.png)

融合结果示例如上。

#### 统计信息展示

除了个人信息的编辑与展示，还可以展示一些所有球员的统计信息。

##### 运动区域热力图

![16](.\pic\16.png)

当在视频展示区右下角下拉菜单选中“heatmap:”时，会在下方区域展示所有球员运动区域的热力图，如图所示，其中颜色更深、更偏红的地方代表运动密集，而透明度低、颜色偏紫的则代表运动轨迹在此比较稀疏。

##### 全员信息对比

![17](.\pic\17.png)

点击信息展示面板中的”comparation”按钮，可以显示全员信息对比的对话框，如图17所示，以柱状图的形式呈现，由于全员信息较多，因此采用可拖动窗口的形式，用户可以通过拖动下方的滚动条左右查看。通过对话框上方的几个选择按钮，可以在最大速度、平均速度、跑动距离三个信息中选择一个数据类型进行查看。

#### 标注框修改

![18](.\pic\18.png)

标注文件的载入与视频载入类似，点击标签编辑面板的”load label”按钮，便可以依次选择由算法生成的txt文件。

标注修改面板如图18所示，点击”edit label”前面的选择框即可开启标签编辑模式。该选择框只在载入标签文件后可用。

开启编辑标签选项，视频自动暂停，自动开启bounding box选项。之后可以对于自动识别出错的情况进行手动修改。在视频播放栏中拖动矩形边界框进行移动，拖拽标注框的角点可以改变标注框的大小，点击某个标注框内部选中该标注框。这些实现也是重载了视频播放窗口的鼠标移动、鼠标点击、鼠标释放按钮。通过鼠标事件中鼠标的位置确定究竟是哪种操作。

在控制面板中，可以方便地切换前一帧、后一帧。也可以保存修改过的标注文件到加载的标注文件。下方的列表窗口中将会显示当前画面中一共有多少个不同id的识别框，点击某一行的id部分可以对选中标注框的id进行修改。此外，列表窗口中的每一行内容除了显示id以外，还显示属于左半场视频还是右半场视频，以及之前在个人球员信息中所添加的个人信息。点击某一行，视频播放窗口中的对应标签也会被选中，高亮显示。

