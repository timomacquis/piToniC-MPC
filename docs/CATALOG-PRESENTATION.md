# Catalog name, description and preview

The Open MPC registry controls these fields in catalog/plugins/<id>.json in open-mpc/mpc-vst-plugins. A registry pull request is required; editing this repository alone does not change the card. No new plugin release is needed.

- Name: name. Keep the technical id, uid, repository and asset_pattern stable.
- Description: summary. Use the English paragraphs from PRODUCT-DESCRIPTION.json as plain text.
- Preview: screenshot. Commit the chosen PNG/JPG here, then use its public raw HTTPS URL pinned to a commit or tag in the registry. The catalog card uses a 2:1 frame and may crop edges; clicking opens the full image.

The existing Cover is retained for this presentation update. Documentation changes do not qualify a new plugin build or alter existing release ZIPs.
