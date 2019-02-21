

cd ~/Documents/Development/git/f100l

git checkout master
git branch -D gh-pages

rm -rf _modules _images _sources _static doc genindex.html index.html objects.inv py-modindex.html search.html searchindex.js .nojekyll

git pull
git checkout -b gh-pages
pushd src
make html
popd
cp -r src/_build/html/* .
cp -r src/_build/html/.nojekyll .
git add _sources _images _static doc genindex.html index.html objects.inv py-modindex.html search.html searchindex.js .nojekyll
git commit -m "checkin documentation"
rm -rf src/_build
git branch --set-upstream-to=origin/gh-pages gh-pages
git push origin gh-pages -f 

# return to master branch on exit
git checkout master

# Checkin all changes (but not newly created files)
# commit -am "message"

# Merge branch develop into master
#git checkout master
#git pull               # to update the state to the latest remote master state
#git merge develop      # to bring changes to local master from your develop branch
#git push origin master # push current HEAD to remote master branch
