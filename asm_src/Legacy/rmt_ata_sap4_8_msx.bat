:LOP
@cls
@xasm.exe rmt_ata.a65
@xasm.exe rmt_sap4.a65
@xasm.exe rmt_sap8.a65
@xasm.exe rmt_msx.a65
@copy rmt_ata.obx rmt_ata.sys
@copy rmt_sap4.obx rmt_sap4.sys
@copy rmt_sap8.obx rmt_sap8.sys
@copy rmt_msx.obx rmt_msx.sys
@pause
@goto LOP