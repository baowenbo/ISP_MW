cd /d "%~dp0"

FOR %%i IN (%*) DO (
    ISP_MW --Retinex_MSRCR --sigma 15 --sigma 80 --lower_thr 0.01 --upper_thr 0.01 --restore 125 %%i
)

pause