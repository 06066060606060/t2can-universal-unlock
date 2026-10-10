(()=>{
  const state=window.__t2PreviewState;
  if(!state)return;
  const q=id=>document.getElementById(id);
  function navigate(values){
    const url=new URL(location.href);
    const next={...state,...values};
    if(next.model===1)next.topology=1;
    else if(next.topology===1)next.topology=2;
    next.turn=next.topology===3?0:next.model===4?(next.turn===2?2:1):1;
    url.searchParams.set('previewAp',next.ap);
    url.searchParams.set('previewModel',String(next.model));
    url.searchParams.set('previewTopology',String(next.topology));
    url.searchParams.set('previewTurn',String(next.turn));
    location.assign(url.href);
  }
  const apButtons=[...document.querySelectorAll('#t2PreviewControls [data-preview-ap]')];
  apButtons.forEach(button=>{
    button.setAttribute('aria-pressed',String(button.dataset.previewAp===state.ap));
    button.onclick=()=>{
      window.__t2PreviewSetAp(button.dataset.previewAp);
      apButtons.forEach(item=>item.setAttribute('aria-pressed',String(item.dataset.previewAp===state.ap)));
      try{const url=new URL(location.href);url.searchParams.set('previewAp',state.ap);history.replaceState(null,'',url.href)}catch(e){}
      if(typeof showPage==='function')showPage('home');
      else if(typeof forcePoll==='function')forcePoll();
    };
  });
  document.querySelectorAll('#t2PreviewControls [data-preview-model]').forEach(button=>{
    const model=Number(button.dataset.previewModel);
    button.setAttribute('aria-pressed',String(model===state.model));
    button.onclick=()=>navigate({model,topology:model===1?1:2,turn:model===4?2:1});
  });
  document.querySelectorAll('#t2PreviewControls [data-preview-topology]').forEach(button=>{
    const topology=Number(button.dataset.previewTopology);
    button.hidden=state.model===1?topology!==1:topology===1;
    button.setAttribute('aria-pressed',String(topology===state.topology));
    button.onclick=()=>navigate({topology,turn:state.model===4?2:1});
  });
  q('previewTurnWrap').hidden=!(state.model===4&&state.topology===2);
  document.querySelectorAll('#t2PreviewControls [data-preview-turn]').forEach(button=>{
    const turn=Number(button.dataset.previewTurn);
    button.setAttribute('aria-pressed',String(turn===state.turn));
    button.onclick=()=>navigate({turn});
  });
})();
