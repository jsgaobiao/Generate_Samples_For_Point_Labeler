[Usage] ./GenerateSamplesForPointLabeler [dsvl] [transproc.log] [calib] [colortable] [output_dir] [bbox] [bbox_overwrite_flag]

参数说明：
[dsvl](required)   例如20190331133302_4-seg.dsvl-timefix
[transproc.log](required): 基于rangeimage的标注结果，例如20190331133302_4-transproc.log
[calib](default): 标定参数，默认是P40n.calib
[colortable](default): 标签和颜色对应的表，默认是colortable.txt
[output_dir](default): 输入结果的文件夹，默认是data_for_point_labeler
[bbox](optional): （可选）3d的包围框，例如bbx-all.log
[bbox_overwrite_flag](optional): （可选，和bbox参数同时存在，If bbox is not empty, must have this parameter.）该值为0表示保留labels_bak中的标签信息(labels_bak文件夹默认在output_dir下)，只覆盖包围框中的标签和instance id； 若该值为1则保留transproc中的标签和3d box中的标签和instance id。
