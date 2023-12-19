const REPOSITORY = 'CommanderRedYT/ws2812-clock';
const REPO_URL = `//api.github.com/repos/${REPOSITORY}`;
const MARKDOWN_URL = '//api.github.com/markdown';

const cacheData = (key, data) => {
    const storage = window.localStorage;
    const cache = {timestamp: Date.now(), data};
    storage.setItem(key, JSON.stringify(cache));
};

const getCachedData = (key) => {
    const storage = window.localStorage;
    const cache = storage.getItem(key);
    if (!cache) return null;
    const { timestamp, data } = JSON.parse(cache);
    if (Date.now() - timestamp > 1000 * 60 * 60 * 24) return null; // 1 day
    return data;
};

const clearCache = (key) => {
    const storage = window.localStorage;
    storage.removeItem(key);
};

const fetchOrGetCached = async (url, fetchParams) => {
    const cached = getCachedData(url);
    if (cached) return cached;
    const response = await fetch(url, fetchParams);
    const data = await response.json();
    cacheData(url, data);
    return data;
};

export const getReleases = async () => {
    return await fetchOrGetCached(`${REPO_URL}/releases`);
};

export const getReleaseDescription = async (release) => {
    const {body} = await fetchOrGetCached(release.url);
    return body;
};

export const renderMarkdown = async (markdown) => {
    // use caching also here
    const cached = getCachedData(markdown);
    if (cached) return cached;

    const response = await fetch(MARKDOWN_URL, {
        method: 'POST',
        headers: {
            'accept': 'application/vnd.github+json',
        },
        body: JSON.stringify({
            text: markdown,
            mode: 'gfm',
            context: REPOSITORY,
        }),
    });

    const html = await response.text();
    cacheData(markdown, html);

    return html;
}

export const createReleaseList = async () => {
    const releases = await getReleases();

    // container is div with class "d-flex flex-wrap"
    const container = document.createElement('div');
    // container.classList.add('d-flex', 'flex-wrap', 'flex-row', 'justify-content-center', 'align-items-center', 'gap-2');
    // make it grid instead
    container.classList.add('row');

    const sortedReleases = releases.sort((a, b) => {
        if (a.published_at > b.published_at) return -1;
        if (a.published_at < b.published_at) return 1;
        return 0;
    });

    for (const index in sortedReleases) {
        const release = sortedReleases[index];
        const isNewest = index === '0';
        const {name, assets} = release;

        const gridItem = document.createElement('div');
        gridItem.classList.add('col-12', 'col-md-6', 'col-lg-4', 'col-xl-3', 'd-flex', 'justify-content-center', 'align-items-stretch', 'p-0');

        const description = await getReleaseDescription(release);

        const nameElement = document.createElement('h3');
        nameElement.innerText = name;

        if (isNewest) {
            const badge = document.createElement('span');
            badge.innerText = 'Newest';
            badge.classList.add('badge', 'bg-success');
            nameElement.appendChild(badge);
        }

        /** @type {{browser_download_url: string; name: string}} */
        const binary = assets.find((asset) => asset.name.endsWith('.bin'));
        const binaryUrl = new URL(binary.browser_download_url);

        // add a "select" button and a "download locally" button
        const selectButton = document.createElement('button');
        selectButton.innerText = 'Select';
        selectButton.classList.add('btn', 'btn-primary');
        selectButton.addEventListener('click', () => {
            const otaUrl = document.getElementById('otaUrl');
            otaUrl.value = binaryUrl;
            otaUrl.dispatchEvent(new Event('change'));
            otaUrl.focus();
        });

        const downloadButton = document.createElement('a');
        downloadButton.innerText = 'Download';
        downloadButton.href = binaryUrl;
        downloadButton.download = binary.name;
        downloadButton.classList.add('btn', 'btn-secondary');

        const buttons = document.createElement('div');
        buttons.classList.add('btn-group', 'w-100');
        buttons.appendChild(selectButton);
        buttons.appendChild(downloadButton);

        const wrapper = document.createElement('div');
        wrapper.classList.add('p-1');

        const releaseContainer = document.createElement('div');
        releaseContainer.classList.add('release', 'card', 'h-100', 'p-2');

        const releaseHeader = document.createElement('div');
        releaseHeader.classList.add('card-header');

        const releaseTitle = document.createElement('div');
        releaseTitle.classList.add('card-title');
        releaseTitle.appendChild(nameElement);

        releaseHeader.appendChild(releaseTitle);

        const releaseBody = document.createElement('div');
        releaseBody.classList.add('card-body', 'd-flex', 'flex-column', 'justify-content-between', 'align-items-start', 'p-2');

        const releaseDescription = document.createElement('div');
        releaseDescription.classList.add('card-text', 'text-wrap', 'text-break', 'd-flex', 'flex-column');
        releaseDescription.innerHTML = await renderMarkdown(description);

        releaseBody.appendChild(releaseDescription);
        releaseBody.appendChild(buttons);

        releaseContainer.appendChild(releaseHeader);
        releaseContainer.appendChild(releaseBody);

        wrapper.appendChild(releaseContainer);

        gridItem.appendChild(wrapper);

        container.appendChild(gridItem);
    }

    const clearCacheButton = document.createElement('button');
    clearCacheButton.innerText = 'Clear Cache';
    clearCacheButton.classList.add('btn', 'btn-danger');
    clearCacheButton.addEventListener('click', () => {
        clearCache(`${REPO_URL}/releases`);
    });

    container.appendChild(clearCacheButton);

    return container;
}
