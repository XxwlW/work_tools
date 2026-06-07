@echo off
REM install-skills.bat — Windows 一键导入 Claude Code skills
REM 用法: install-skills.bat [target_dir]
REM   默认目标: %USERPROFILE%\.claude\skills

setlocal enabledelayedexpansion

set "TARGET_DIR=%USERPROFILE%\.claude\skills"
if not "%~1"=="" set "TARGET_DIR=%~1"

set "SCRIPT_DIR=%~dp0"
set "SRC_GLOBAL=%SCRIPT_DIR%global-skills"
set "SRC_LOCAL=%SCRIPT_DIR%guizang-social-card-skill"

if not exist "%SRC_GLOBAL%" (
  echo ❌ 错误: 找不到 global-skills 目录: %SRC_GLOBAL%
  exit /b 1
)

if not exist "%TARGET_DIR%" mkdir "%TARGET_DIR%"

echo 📦 正在导入 global-skills 到: %TARGET_DIR%
set COUNT=0

for /d %%D in ("%SRC_GLOBAL%\*") do (
  set "SKILL_NAME=%%~nxD"
  if exist "%TARGET_DIR%\!SKILL_NAME!" (
    echo   ⚠️  已存在: !SKILL_NAME!（跳过）
  ) else (
    xcopy /E /I /Y "%%D" "%TARGET_DIR%\!SKILL_NAME!" >nul
    echo   ✅ 安装: !SKILL_NAME!
    set /a COUNT+=1
  )
)

if exist "%SRC_LOCAL%" (
  if exist "%TARGET_DIR%\guizang-social-card-skill" (
    echo   ⚠️  已存在: guizang-social-card-skill（跳过）
  ) else (
    xcopy /E /I /Y "%SRC_LOCAL%" "%TARGET_DIR%\guizang-social-card-skill" >nul
    echo   ✅ 安装: guizang-social-card-skill
    set /a COUNT+=1
  )
)

echo.
echo ✨ 完成！共安装 !COUNT! 个 skill。
echo 📂 目标位置: %TARGET_DIR%
echo.
echo 重启 Claude Code 让 skills 生效。
endlocal
