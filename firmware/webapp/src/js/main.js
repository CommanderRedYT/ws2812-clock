import { ClockApi } from './api.js';

window.addEventListener('load', () => {
    const isDev = window.location.hostname === 'localhost';
    window.clockApi = new ClockApi(isDev);

    window.clockApi.on('onStatusChange', status => {
        console.log('onStatusChange', status);
        document.getElementById('status').innerText = JSON.stringify(status, null, 2);
    });

    window.clockApi.on('onAnimationsChange', animations => {
        console.log('onAnimationsChange', animations);
        document.getElementById('animations').innerText = JSON.stringify(animations, null, 2);
    });

    window.clockApi.on('onOtaStatusChange', animations => {
        console.log('onOtaStatusChange', animations);
        document.getElementById('ota').innerText = JSON.stringify(animations, null, 2);
    });

    window.clockApi.on('onConfigChange', config => {
        console.log('onConfigChange', config);
        document.getElementById('config').innerText = JSON.stringify(config, null, 2);

        // <select name="key" id="key"></select>
        const select = document.getElementById('key');
        const selected = select.value;
        select.innerHTML = '';
        Object.keys(config).forEach(key => {
            const option = document.createElement('option');
            option.value = key;
            option.innerText = key;
            select.appendChild(option);
        });
        select.value = selected;
    });

    const form = document.getElementById('setKey');
    form.addEventListener('submit', e => {
        e.preventDefault();
        const key = document.getElementById('key').value;
        const value = document.getElementById('value').value;
        window.clockApi.setConfig({[key]: JSON.parse(value)});
    });

    const select = document.getElementById('key');
    select.addEventListener('change', e => {
        const key = e.target.value;
        const value = window.clockApi.config[key];
        document.getElementById('value').value = JSON.stringify(value, null, 2);
    });
});
