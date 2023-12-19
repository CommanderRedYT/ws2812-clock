const keyMap = {
    'date': 'Date',
    'version': 'Version',
    'idf_ver': 'IDF Version',
    'project_name': 'App Name',
    'secure_version': 'Secure Version',
    'time': 'Time',
    'magic_word': 'Magic Word',
};

export const generateCurrentOtaPartitionHtml = (currentOtaPartition) => {
    const card = document.createElement('div');
    card.classList.add('card', 'mb-3');

    const cardHeader = document.createElement('div');
    cardHeader.classList.add('card-header', 'text-white', 'bg-success');
    cardHeader.innerText = 'Current OTA Partition';
    card.appendChild(cardHeader);

    const html = document.createElement('div');
    html.classList.add('card-body');

    for (const [key, value] of Object.entries(currentOtaPartition)) {
        let text = key in keyMap ? keyMap[key] : key;
        text += ': ' + value;

        const p = document.createElement('p');
        p.innerText = text;
        html.appendChild(p);
    }

    card.appendChild(html);

    return card.outerHTML;
};

export const generateOtherOtaPartitionHtml = (otherOtaPartition) => {
    const card = document.createElement('div');
    card.classList.add('card', 'mb-3');

    const cardHeader = document.createElement('div');
    cardHeader.classList.add('card-header', 'text-white', 'bg-danger');
    cardHeader.innerText = 'Other OTA Partition';
    card.appendChild(cardHeader);

    const html = document.createElement('div');
    html.classList.add('card-body');

    for (const [key, value] of Object.entries(otherOtaPartition)) {
        let text = key in keyMap ? keyMap[key] : key;
        text += ': ' + value;

        const p = document.createElement('p');
        p.innerText = text;
        html.appendChild(p);
    }

    card.appendChild(html);

    return card.outerHTML;
};
