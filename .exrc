let s:cpo_save=&cpo
set cpo&vim
inoremap <C-W> u
inoremap <C-U> u
nnoremap  :tabp
nnoremap  :tabn
nnoremap  :tabnew **/*
nmap  d
nnoremap <silent>  :nohlsearch
omap <silent> % <Plug>(MatchitOperationForward)
xmap <silent> % <Plug>(MatchitVisualForward)
nmap <silent> % <Plug>(MatchitNormalForward)
nnoremap & :&&
tnoremap , 
nnoremap ,t :below 10split | terminal 
nnoremap ,) :tabmove 9
nnoremap ,( :tabmove 8
nnoremap ,/ :tabmove 7
nnoremap ,& :tabmove 6
nnoremap ,% :tabmove 5
nnoremap ,$ :tabmove 4
nnoremap ,¬£ :tabmove 3
nnoremap ," :tabmove 2
nnoremap ,! :tabmove 1
nnoremap ,= :tabmove 0
nnoremap ,9 9gt
nnoremap ,8 8gt
nnoremap ,7 7gt
nnoremap ,6 6gt
nnoremap ,5 5gt
nnoremap ,4 4gt
nnoremap ,3 3gt
nnoremap ,2 2gt
nnoremap ,1 1gt
nnoremap ,e :20vs .
xnoremap <silent> <expr> @ mode() ==# 'V' ? ':normal! @'.getcharstr().'' : '@'
nnoremap H :tabmove -1
nnoremap L :tabmove +1
xnoremap <silent> <expr> Q mode() ==# 'V' ? ':normal! @=reg_recorded()' : 'Q'
nnoremap Y y$
omap <silent> [% <Plug>(MatchitOperationMultiBackward)
xmap <silent> [% <Plug>(MatchitVisualMultiBackward)
nmap <silent> [% <Plug>(MatchitNormalMultiBackward)
omap <silent> ]% <Plug>(MatchitOperationMultiForward)
xmap <silent> ]% <Plug>(MatchitVisualMultiForward)
nmap <silent> ]% <Plug>(MatchitNormalMultiForward)
xmap a% <Plug>(MatchitVisualTextObject)
omap <silent> g% <Plug>(MatchitOperationBackward)
xmap <silent> g% <Plug>(MatchitVisualBackward)
nmap <silent> g% <Plug>(MatchitNormalBackward)
xmap <silent> <Plug>(MatchitVisualTextObject) <Plug>(MatchitVisualMultiBackward)o<Plug>(MatchitVisualMultiForward)
onoremap <silent> <Plug>(MatchitOperationMultiForward) :call matchit#MultiMatch("W",  "o")
onoremap <silent> <Plug>(MatchitOperationMultiBackward) :call matchit#MultiMatch("bW", "o")
xnoremap <silent> <Plug>(MatchitVisualMultiForward) :call matchit#MultiMatch("W",  "n")m'gv``
xnoremap <silent> <Plug>(MatchitVisualMultiBackward) :call matchit#MultiMatch("bW", "n")m'gv``
nnoremap <silent> <Plug>(MatchitNormalMultiForward) :call matchit#MultiMatch("W",  "n")
nnoremap <silent> <Plug>(MatchitNormalMultiBackward) :call matchit#MultiMatch("bW", "n")
onoremap <silent> <Plug>(MatchitOperationBackward) :call matchit#Match_wrapper('',0,'o')
onoremap <silent> <Plug>(MatchitOperationForward) :call matchit#Match_wrapper('',1,'o')
xnoremap <silent> <Plug>(MatchitVisualBackward) :call matchit#Match_wrapper('',0,'v')m'gv``
xnoremap <silent> <Plug>(MatchitVisualForward) :call matchit#Match_wrapper('',1,'v'):if col("''") != col("$") | exe ":normal! m'" | endifgv``
nnoremap <silent> <Plug>(MatchitNormalBackward) :call matchit#Match_wrapper('',0,'n')
nnoremap <silent> <Plug>(MatchitNormalForward) :call matchit#Match_wrapper('',1,'n')
nnoremap <C-P> :tabnew **/*
nnoremap <C-H> :tabp
nmap <C-W><C-D> d
nnoremap <C-L> :tabn
inoremap  u
inoremap  u
inoremap " ""<Left>
inoremap ' ''<Left>
inoremap ( ()<Left>
inoremap [ []<Left>
inoremap ` ``<Left>
inoremap { {}O
let &cpo=s:cpo_save
unlet s:cpo_save
set clipboard=unnamedplus
set expandtab
set helplang=en
set listchars=eol:‚Ü≤,nbsp:‚ê£,space:¬∑,tab:‚Üí\ ,trail:‚Ä¢
set runtimepath=~/.config/nvim,/etc/xdg/nvim,~/.local/share/nvim/site,/usr/local/share/nvim/site,/usr/share/nvim/site,/var/lib/snapd/desktop/nvim/site,/usr/share/nvim/runtime,/usr/share/nvim/runtime/pack/dist/opt/matchit,/usr/lib/nvim,/var/lib/snapd/desktop/nvim/site/after,/usr/share/nvim/site/after,/usr/local/share/nvim/site/after,~/.local/share/nvim/site/after,/etc/xdg/nvim/after,~/.config/nvim/after
set shiftwidth=4
set tabstop=4
set termguicolors
set window=47
" vim: set ft=vim :
