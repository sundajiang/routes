#make sure that you have built a remote repositorie already
git init
git add -A
git commit -m "旧开发板路由程序存档"

git remote add origin git@github.com:sundajiang/routes.git
git push -u origin master
