
add_subdirectory(TTXAlwaysOnTop)
set_target_properties(TTXAlwaysOnTop PROPERTIES FOLDER TTXSamples)

add_subdirectory(TTXCallSysMenu)
set_target_properties(TTXCallSysMenu PROPERTIES FOLDER TTXSamples)

add_subdirectory(TTXCommandLineOpt)
set_target_properties(TTXCommandLineOpt PROPERTIES FOLDER TTXSamples)

add_subdirectory(TTXCopyIniFile)
set_target_properties(TTXCopyIniFile PROPERTIES FOLDER TTXSamples)

# ウィンドウサイズを 80x24 に強制するデモ。リサイズ不可になり公式配布にも
# 含まれないため、ビルド対象から除外する。
#add_subdirectory(TTXFixedWinSize)
#set_target_properties(TTXFixedWinSize PROPERTIES FOLDER TTXSamples)

add_subdirectory(TTXKcodeChange)
set_target_properties(TTXKcodeChange PROPERTIES FOLDER TTXSamples)

add_subdirectory(TTXOutputBuffering)
set_target_properties(TTXOutputBuffering PROPERTIES FOLDER TTXSamples)

add_subdirectory(TTXRecurringCommand)
set_target_properties(TTXRecurringCommand PROPERTIES FOLDER TTXSamples)

add_subdirectory(TTXResizeMenu)
set_target_properties(TTXResizeMenu PROPERTIES FOLDER TTXSamples)

add_subdirectory(TTXResizeWin)
set_target_properties(TTXResizeWin PROPERTIES FOLDER TTXSamples)

# 起動毎にコマンドラインを警告ボックスで表示するだけのデモ。公式配布物にも含まれない
# ため、ビルド対象から除外する。
#add_subdirectory(TTXShowCommandLine)
#set_target_properties(TTXShowCommandLine PROPERTIES FOLDER TTXSamples)

# テスト用プラグイン。公式配布物に含まれないため、ビルド対象から除外する。
#add_subdirectory(ttxtest)
#set_target_properties(TTXtest PROPERTIES FOLDER TTXSamples)

add_subdirectory(TTXttyrec)
set_target_properties(TTXttyrec PROPERTIES FOLDER TTXSamples)
set_target_properties(TTXttyplay PROPERTIES FOLDER TTXSamples)

add_subdirectory(TTXViewMode)
set_target_properties(TTXViewMode PROPERTIES FOLDER TTXSamples)

add_subdirectory(TTXCheckUpdate)
set_target_properties(TTXCheckUpdate PROPERTIES FOLDER TTXSamples)

add_subdirectory(TTXChangeFontSize)
set_target_properties(TTXChangeFontSize PROPERTIES FOLDER TTXSamples)
